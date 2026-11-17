// kscc: gerador de código para a KSVM v2. Derivado do codegen.c do chibicc
// (Rui Ueyama, MIT), com a mesma estrutura e os mesmos nomes de função
// (doc 03). O que muda é o texto emitido: .ksasm do ksas em vez de
// assembly x86-64 do GNU as.
//
// Invariantes (iguais aos do chibicc, com registradores trocados):
//  - gen_expr() deixa o valor em r1 (era %rax). char/short/_Bool chegam
//    sempre estendidos a 32 bits, então r1 nunca tem "lixo" nos bits altos
//    (no x86-64 os 32 bits altos de %rax podiam ter).
//  - gen_addr() deixa o endereço em r1.
//  - push()/pop() empilham 32 bits; `depth` conta as palavras empilhadas.
//  - binários: gen_expr(rhs); push; gen_expr(lhs); pop r2 -> r1 op r2.
//
// Não suportado nesta versão (erro de compilação, nunca código errado):
// ponto flutuante, long long, struct/union por valor em chamadas e
// retornos (fase 2), alloca/VLA (fase 3), TLS, atômicos e asm.

#include "chibicc.h"

// kscc: quatro registradores de argumento (doc 02 §3), sem classe de ponto
// flutuante.
#define GP_MAX 4

static FILE *output_file;
static int depth;
static char *argreg[] = {"r1", "r2", "r3", "r4"};
static Obj *current_fn;

// kscc: prefixo de todos os labels gerados: "L<id da unidade>". O ksas lê
// ".L..." como diretiva, e os labels precisam ser únicos entre unidades
// concatenadas no link (doc 04 §3).
static char *L;

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

__attribute__((format(printf, 1, 2)))
static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

static int count(void) {
  static int i = 1;
  return i++;
}

// kscc: `.loc` do chibicc vira comentário `; arquivo:linha` (o ksas não tem
// informação de depuração de linha). Só emitido quando a linha muda.
static void emit_loc(Token *tok) {
  static File *last_file;
  static int last_line;

  if (!tok || !tok->file)
    return;
  if (tok->file == last_file && tok->line_no == last_line)
    return;
  last_file = tok->file;
  last_line = tok->line_no;
  println("; %s:%d", tok->file->name, tok->line_no);
}

static void push(void) {
  println("  push r1");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

// kscc: imediatos de addi/ldw/stw têm 14 bits com sinal (−8192..8191);
// fora disso usa-se a forma L (imediato de 32 bits).
static bool is_imm14(long v) {
  return -8192 <= v && v <= 8191;
}

// rd = rs + imm
static void add_imm(char *rd, char *rs, int imm) {
  if (imm == 0) {
    if (strcmp(rd, rs))
      println("  mov %s, %s", rd, rs);
    return;
  }
  if (is_imm14(imm))
    println("  addi %s, %s, %d", rd, rs, imm);
  else
    println("  addl %s, %s, %d", rd, rs, imm);
}

// kscc: nome do símbolo no assembly. Objetos static já foram renomeados
// para _S<id>_nome em mangle_static_symbols().
static char *sym(Obj *var) {
  return var->name;
}

// kscc: erro explícito para tipos que a v1 não gera (doc 03 §2).
static void check_type(Node *node) {
  Type *ty = node->ty;
  if (!ty)
    return;
  if (is_flonum(ty))
    error_tok(node->tok, "ponto flutuante nao suportado nesta versao do kscc");
  if (is_integer(ty) && ty->size > 4)
    error_tok(node->tok, "long long nao suportado nesta versao do kscc");
}

static bool is_struct(Type *ty) {
  return ty->kind == TY_STRUCT || ty->kind == TY_UNION;
}

// kscc: a convenção variádica da KSVM (tudo na pilha) só vale para
// protótipos com "...". Uma declaração sem protótipo `f()` usa a normal.
static bool is_abi_variadic(Type *fn_ty) {
  return fn_ty->is_variadic && !fn_ty->is_oldstyle;
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR:
    // Variable-length array, which is always local.
    if (node->var->ty->kind == TY_VLA)
      error_tok(node->tok, "VLA nao suportado nesta versao do kscc");

    // Local variable
    if (node->var->is_local) {
      add_imm("r1", "fp", node->var->offset);
      return;
    }

    // Thread-local variable
    if (node->var->is_tls)
      error_tok(node->tok, "_Thread_local nao suportado nesta versao do kscc");

    // kscc: função ou global: endereço absoluto. `la` é a pseudo-instrução
    // do ksas (MOVIL; ADRL com .pic). Não há GOT nem PLT: o link por
    // concatenação resolve tudo.
    println("  la r1, %s", sym(node->var));
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  case ND_COMMA:
    gen_expr(node->lhs);
    gen_addr(node->rhs);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs);
    add_imm("r1", "r1", node->member->offset);
    return;
  case ND_FUNCALL:
    if (node->ret_buffer) {
      gen_expr(node);
      return;
    }
    break;
  case ND_ASSIGN:
  case ND_COND:
    if (is_struct(node->ty)) {
      gen_expr(node);
      return;
    }
    break;
  case ND_VLA_PTR:
    error_tok(node->tok, "VLA nao suportado nesta versao do kscc");
  }

  error_tok(node->tok, "not an lvalue");
}

// Load a value from where r1 is pointing to.
static void load(Type *ty) {
  switch (ty->kind) {
  case TY_ARRAY:
  case TY_STRUCT:
  case TY_UNION:
  case TY_FUNC:
  case TY_VLA:
    // If it is an array, do not attempt to load a value to the
    // register because in general we can't load an entire array to a
    // register. As a result, the result of an evaluation of an array
    // becomes not the array itself but the address of the array.
    // This is where "array is automatically converted to a pointer to
    // the first element of the array in C" occurs.
    return;
  case TY_FLOAT:
  case TY_DOUBLE:
  case TY_LDOUBLE:
    unreachable();
  }

  // kscc: os loads da KSVM já estendem para 32 bits (ldbs/ldhs com sinal,
  // ldb/ldh com zero), mantendo o invariante de que r1 está sempre
  // completo.
  char *u = ty->is_unsigned ? "" : "s";

  if (ty->size == 1)
    println("  ldb%s r1, [r1]", u);
  else if (ty->size == 2)
    println("  ldh%s r1, [r1]", u);
  else if (ty->size == 4)
    println("  ldw r1, [r1]");
  else
    unreachable();
}

// kscc: copia `size` bytes de [src] para [dst] (struct/union). Até 32
// bytes desenrolado; acima, laço palavra a palavra com r5 como contador
// (doc 03, store). Destrói r3, r5 e avança src/dst no caso do laço.
static void copy_bytes(char *src, char *dst, int size) {
  int i = 0;

  if (size > 32) {
    int c = count();
    println("  li r5, %d", size / 4);
    println("%s.copy.%d:", L, c);
    println("  ldw r3, [%s]", src);
    println("  stw [%s], r3", dst);
    println("  addi %s, %s, 4", src, src);
    println("  addi %s, %s, 4", dst, dst);
    println("  subi r5, r5, 1");
    println("  bne %s.copy.%d", L, c);
    for (i = 0; i < size % 4; i++) {
      println("  ldb r3, [%s + %d]", src, i);
      println("  stb [%s + %d], r3", dst, i);
    }
    return;
  }

  for (; i + 4 <= size; i += 4) {
    println("  ldw r3, [%s + %d]", src, i);
    println("  stw [%s + %d], r3", dst, i);
  }
  for (; i + 2 <= size; i += 2) {
    println("  ldh r3, [%s + %d]", src, i);
    println("  sth [%s + %d], r3", dst, i);
  }
  for (; i < size; i++) {
    println("  ldb r3, [%s + %d]", src, i);
    println("  stb [%s + %d], r3", dst, i);
  }
}

// Store r1 to an address that the stack top is pointing to.
static void store(Type *ty) {
  pop("r2");

  switch (ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    // O valor de uma expressão struct é o seu endereço: r1 tem que
    // continuar apontando para a origem depois da cópia.
    println("  mov r6, r1");
    copy_bytes("r1", "r2", ty->size);
    println("  mov r1, r6");
    return;
  case TY_FLOAT:
  case TY_DOUBLE:
  case TY_LDOUBLE:
    unreachable();
  }

  if (ty->size == 1)
    println("  stb [r2], r1");
  else if (ty->size == 2)
    println("  sth [r2], r1");
  else if (ty->size == 4)
    println("  stw [r2], r1");
  else
    unreachable();
}

// kscc: `cmp r1, zr` deixa Z = (r1 == 0). Como r1 está sempre estendido a
// 32 bits, não há distinção de tamanho (o chibicc escolhia eax/rax).
static void cmp_zero(Type *ty) {
  if (is_flonum(ty))
    unreachable();
  println("  cmp r1, zr");
}

// kscc: sem SETcc na KSVM: materializa 0/1 com movi + desvio condicional.
// `movi` não altera as flags, então o desvio ainda vê as do `cmp`.
static void set_cond(char *cc) {
  int c = count();
  println("  movi r1, 1");
  println("  b%s %s.set.%d", cc, L, c);
  println("  movi r1, 0");
  println("%s.set.%d:", L, c);
}

// kscc: só as classes inteiras de até 32 bits (doc 03, cast). Ponteiros e
// enums são U32/I32.
enum { I8, I16, I32, U8, U16, U32 };

static int getTypeId(Type *ty) {
  switch (ty->kind) {
  case TY_BOOL:
    return U8;
  case TY_CHAR:
    return ty->is_unsigned ? U8 : I8;
  case TY_SHORT:
    return ty->is_unsigned ? U16 : I16;
  case TY_INT:
  case TY_LONG:
  case TY_ENUM:
    return ty->is_unsigned ? U32 : I32;
  }
  return U32;
}

// The table for type casts
static char i32i8[] = "sxb r1, r1";
static char i32u8[] = "zxb r1, r1";
static char i32i16[] = "sxh r1, r1";
static char i32u16[] = "zxh r1, r1";

static char *cast_table[][6] = {
  // i8   i16     i32   u8     u16     u32
  {NULL,  NULL,   NULL, i32u8, i32u16, NULL}, // i8
  {i32i8, NULL,   NULL, i32u8, i32u16, NULL}, // i16
  {i32i8, i32i16, NULL, i32u8, i32u16, NULL}, // i32
  {i32i8, NULL,   NULL, NULL,  NULL,   NULL}, // u8
  {i32i8, i32i16, NULL, i32u8, NULL,   NULL}, // u16
  {i32i8, i32i16, NULL, i32u8, i32u16, NULL}, // u32
};

static void cast(Type *from, Type *to) {
  if (to->kind == TY_VOID)
    return;

  if (to->kind == TY_BOOL) {
    cmp_zero(from);
    set_cond("ne");
    return;
  }

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);
  if (cast_table[t1][t2])
    println("  %s", cast_table[t1][t2]);
}

// kscc: classificação dos argumentos (doc 02 §3.1): os 4 primeiros
// escalares vão em r1-r4, o resto na pilha; em chamadas variádicas, todos
// na pilha. Structs por valor ficam para a fase 2.
static void push_args2(Node *args, bool first_pass) {
  if (!args)
    return;
  push_args2(args->next, first_pass);

  if ((first_pass && !args->pass_by_stack) || (!first_pass && args->pass_by_stack))
    return;

  gen_expr(args);
  push();
}

// Load function call arguments. Arguments are evaluated right to left
// and pushed; the ones passed by register are pushed last (second pass)
// so that they can be popped to r1..r4 right before the call, leaving the
// stack arguments in place with the leftmost one at the lowest address.
static int push_args(Node *node) {
  int stack = 0, gp = 0;
  bool variadic = is_abi_variadic(node->func_ty);

  for (Node *arg = node->args; arg; arg = arg->next) {
    if (is_struct(arg->ty))
      error_tok(arg->tok, "struct/union passada por valor nao suportada "
                "nesta versao do kscc");

    if (variadic || gp++ >= GP_MAX) {
      arg->pass_by_stack = true;
      stack++;
    }
  }

  push_args2(node->args, true);
  push_args2(node->args, false);
  return stack;
}

// Generate code for a given node.
static void gen_expr(Node *node) {
  emit_loc(node->tok);
  check_type(node);

  switch (node->kind) {
  case ND_NULL_EXPR:
    return;
  case ND_NUM:
    // kscc: `li` é pseudo do ksas: MOVI se cabe em 14 bits, senão MOVIL.
    // O valor é reduzido a 32 bits (Node::val é int64_t).
    println("  li r1, %d", (int32_t)(uint32_t)node->val);
    return;
  case ND_NEG:
    gen_expr(node->lhs);
    println("  neg r1, r1");
    return;
  case ND_VAR:
    gen_addr(node);
    load(node->ty);
    return;
  case ND_MEMBER: {
    gen_addr(node);
    load(node->ty);

    // kscc: extração de bitfield em 32 bits (o chibicc usava 64).
    Member *mem = node->member;
    if (mem->is_bitfield) {
      println("  shli r1, r1, %d", 32 - mem->bit_width - mem->bit_offset);
      if (mem->ty->is_unsigned)
        println("  shri r1, r1, %d", 32 - mem->bit_width);
      else
        println("  sari r1, r1, %d", 32 - mem->bit_width);
    }
    return;
  }
  case ND_DEREF:
    gen_expr(node->lhs);
    load(node->ty);
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);

    if (node->lhs->kind == ND_MEMBER && node->lhs->member->is_bitfield) {
      // r6 guarda o valor novo, que é o resultado da expressão.
      println("  mov r6, r1");

      // If the lhs is a bitfield, we need to read the current value
      // from memory and merge it with a new value.
      Member *mem = node->lhs->member;
      uint32_t mask = (uint32_t)(((uint64_t)1 << mem->bit_width) - 1);
      println("  andl r2, r1, 0x%x", mask);
      println("  shli r2, r2, %d", mem->bit_offset);

      // Endereço no topo da pilha, lido sem desempilhar.
      println("  ldw r1, [sp]");
      load(mem->ty);

      println("  andl r1, r1, 0x%x", ~(mask << mem->bit_offset));
      println("  or r1, r1, r2");
      store(node->ty);
      println("  mov r1, r6");
      return;
    }

    store(node->ty);
    return;
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  case ND_COMMA:
    gen_expr(node->lhs);
    gen_expr(node->rhs);
    return;
  case ND_CAST:
    gen_expr(node->lhs);
    cast(node->lhs->ty, node->ty);
    return;
  case ND_MEMZERO: {
    // kscc: sem `rep stosb`; zera com o registrador zr (doc 03,
    // ND_MEMZERO). r2 = endereço, r3 = contador no laço.
    int size = node->var->ty->size;
    int i = 0;
    add_imm("r2", "fp", node->var->offset);
    if (size > 64) {
      int c = count();
      println("  li r3, %d", size / 4);
      println("%s.zero.%d:", L, c);
      println("  stw [r2], zr");
      println("  addi r2, r2, 4");
      println("  subi r3, r3, 1");
      println("  bne %s.zero.%d", L, c);
      for (i = 0; i < size % 4; i++)
        println("  stb [r2 + %d], zr", i);
      return;
    }
    for (; i + 4 <= size; i += 4)
      println("  stw [r2 + %d], zr", i);
    for (; i < size; i++)
      println("  stb [r2 + %d], zr", i);
    return;
  }
  case ND_COND: {
    int c = count();
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  beq %s.else.%d", L, c);
    gen_expr(node->then);
    println("  jmp %s.end.%d", L, c);
    println("%s.else.%d:", L, c);
    gen_expr(node->els);
    println("%s.end.%d:", L, c);
    return;
  }
  case ND_NOT:
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    set_cond("eq");
    return;
  case ND_BITNOT:
    gen_expr(node->lhs);
    println("  not r1, r1");
    return;
  case ND_LOGAND: {
    int c = count();
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    println("  beq %s.false.%d", L, c);
    gen_expr(node->rhs);
    cmp_zero(node->rhs->ty);
    println("  beq %s.false.%d", L, c);
    println("  movi r1, 1");
    println("  jmp %s.end.%d", L, c);
    println("%s.false.%d:", L, c);
    println("  movi r1, 0");
    println("%s.end.%d:", L, c);
    return;
  }
  case ND_LOGOR: {
    int c = count();
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    println("  bne %s.true.%d", L, c);
    gen_expr(node->rhs);
    cmp_zero(node->rhs->ty);
    println("  bne %s.true.%d", L, c);
    println("  movi r1, 0");
    println("  jmp %s.end.%d", L, c);
    println("%s.true.%d:", L, c);
    println("  movi r1, 1");
    println("%s.end.%d:", L, c);
    return;
  }
  case ND_FUNCALL: {
    if (node->lhs->kind == ND_VAR && !strcmp(node->lhs->var->name, "alloca"))
      error_tok(node->tok, "alloca nao suportado nesta versao do kscc");

    if (node->ret_buffer)
      error_tok(node->tok, "retorno de struct/union nao suportado nesta "
                "versao do kscc");

    int stack_args = push_args(node);

    // kscc: chamada direta quando o alvo é uma função com nome; senão o
    // endereço (calculado depois dos argumentos, como no chibicc) vai
    // para r5, que os pops de r1-r4 não tocam (era %r10).
    bool direct = node->lhs->kind == ND_VAR && node->lhs->ty->kind == TY_FUNC;
    if (!direct) {
      gen_expr(node->lhs);
      println("  mov r5, r1");
    }

    int gp = 0;
    for (Node *arg = node->args; arg; arg = arg->next)
      if (!arg->pass_by_stack)
        pop(argreg[gp++]);

    if (direct)
      println("  call %s", sym(node->lhs->var));
    else
      println("  callr r5");

    if (stack_args > 0)
      add_imm("sp", "sp", stack_args * 4);

    depth -= stack_args;

    // kscc: o chamado já estende char/short/_Bool (doc 03, passo 7), mas
    // funções em .ksasm escritas à mão podem não fazê-lo; re-estender
    // custa uma instrução.
    switch (node->ty->kind) {
    case TY_BOOL:
      println("  zxb r1, r1");
      return;
    case TY_CHAR:
      println("  %s r1, r1", node->ty->is_unsigned ? "zxb" : "sxb");
      return;
    case TY_SHORT:
      println("  %s r1, r1", node->ty->is_unsigned ? "zxh" : "sxh");
      return;
    }
    return;
  }
  case ND_LABEL_VAL:
    println("  la r1, %s", node->unique_label);
    return;
  case ND_CAS:
  case ND_EXCH:
    error_tok(node->tok, "atomicos nao suportados nesta versao do kscc");
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop("r2");

  // kscc: tudo em 32 bits; não há a escolha eax/rax do chibicc.
  switch (node->kind) {
  case ND_ADD:
    println("  add r1, r1, r2");
    return;
  case ND_SUB:
    println("  sub r1, r1, r2");
    return;
  case ND_MUL:
    println("  mul r1, r1, r2");
    return;
  case ND_DIV:
    // Divisão por zero: FAULT_DIV_ZERO da VM. INT_MIN / -1 = INT_MIN.
    println("  %s r1, r1, r2", node->ty->is_unsigned ? "divu" : "div");
    return;
  case ND_MOD:
    println("  %s r1, r1, r2", node->ty->is_unsigned ? "remu" : "rem");
    return;
  case ND_BITAND:
    println("  and r1, r1, r2");
    return;
  case ND_BITOR:
    println("  or r1, r1, r2");
    return;
  case ND_BITXOR:
    println("  xor r1, r1, r2");
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    println("  cmp r1, r2");

    if (node->kind == ND_EQ)
      set_cond("eq");
    else if (node->kind == ND_NE)
      set_cond("ne");
    else if (node->kind == ND_LT)
      set_cond(node->lhs->ty->is_unsigned ? "lo" : "lt");
    else
      set_cond(node->lhs->ty->is_unsigned ? "ls" : "le");
    return;
  case ND_SHL:
    // A VM usa só os 5 bits baixos da contagem (como o x86 com %cl).
    println("  shl r1, r1, r2");
    return;
  case ND_SHR:
    if (node->lhs->ty->is_unsigned)
      println("  shr r1, r1, r2");
    else
      println("  sar r1, r1, r2");
    return;
  }

  error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node) {
  emit_loc(node->tok);

  switch (node->kind) {
  case ND_IF: {
    int c = count();
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  beq %s.else.%d", L, c);
    gen_stmt(node->then);
    println("  jmp %s.end.%d", L, c);
    println("%s.else.%d:", L, c);
    if (node->els)
      gen_stmt(node->els);
    println("%s.end.%d:", L, c);
    return;
  }
  case ND_FOR: {
    int c = count();
    if (node->init)
      gen_stmt(node->init);
    println("%s.begin.%d:", L, c);
    if (node->cond) {
      gen_expr(node->cond);
      cmp_zero(node->cond->ty);
      println("  beq %s", node->brk_label);
    }
    gen_stmt(node->then);
    println("%s:", node->cont_label);
    if (node->inc)
      gen_expr(node->inc);
    println("  jmp %s.begin.%d", L, c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_DO: {
    int c = count();
    println("%s.begin.%d:", L, c);
    gen_stmt(node->then);
    println("%s:", node->cont_label);
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  bne %s.begin.%d", L, c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_SWITCH:
    gen_expr(node->cond);

    // kscc: cadeia de comparações como no chibicc (tabela de saltos é a
    // fase 7). Os valores dos case são reduzidos a 32 bits.
    for (Node *n = node->case_next; n; n = n->case_next) {
      int32_t begin = (int32_t)(uint32_t)n->begin;

      if (n->begin == n->end) {
        println("  %s r1, %d", is_imm14(begin) ? "cmpi" : "cmpl", begin);
        println("  beq %s", n->label);
        continue;
      }

      // [GNU] Case ranges: (x - begin) <= (end - begin), sem sinal.
      int32_t span = (int32_t)(uint32_t)(n->end - n->begin);
      add_imm("r2", "r1", -begin);
      println("  %s r2, %d", is_imm14(span) ? "cmpi" : "cmpl", span);
      println("  bls %s", n->label);
    }

    if (node->default_case)
      println("  jmp %s", node->default_case->label);

    println("  jmp %s", node->brk_label);
    gen_stmt(node->then);
    println("%s:", node->brk_label);
    return;
  case ND_CASE:
    println("%s:", node->label);
    gen_stmt(node->lhs);
    return;
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  case ND_GOTO:
    println("  jmp %s", node->unique_label);
    return;
  case ND_GOTO_EXPR:
    gen_expr(node->lhs);
    println("  jmpr r1");
    return;
  case ND_LABEL:
    println("%s:", node->unique_label);
    gen_stmt(node->lhs);
    return;
  case ND_RETURN:
    if (node->lhs) {
      if (is_struct(node->lhs->ty))
        error_tok(node->tok, "retorno de struct/union nao suportado nesta "
                  "versao do kscc");
      gen_expr(node->lhs);
    }

    println("  jmp %s.return.%s", L, current_fn->name);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    return;
  case ND_ASM:
    error_tok(node->tok, "asm nao suportado nesta versao do kscc");
  }

  error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
//
// kscc: frame da KSVM (doc 02 §4): [fp] = fp do chamador, [fp+4] = endereço
// de retorno, argumentos na pilha a partir de fp+8, um por palavra. Os que
// chegam em r1-r4 são copiados para slots locais no prólogo.
static void assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    int top = 8;
    int bottom = 0;
    int gp = 0;
    bool variadic = is_abi_variadic(fn->ty);

    // Assign offsets to pass-by-stack parameters.
    for (Obj *var = fn->params; var; var = var->next) {
      if (is_struct(var->ty))
        error_tok(fn->ty->name, "parametro struct/union por valor nao "
                  "suportado nesta versao do kscc");

      if (!variadic && gp++ < GP_MAX)
        continue;

      var->offset = top;
      top += 4;
    }

    // Assign offsets to pass-by-register parameters and local variables.
    for (Obj *var = fn->locals; var; var = var->next) {
      if (var->offset)
        continue;

      // kscc: fp só é garantidamente múltiplo de 4; alinhar além disso
      // exigiria ajustar sp no prólogo (fase 2).
      if (var->align > 4)
        error_tok(var->ty->name ? var->ty->name : fn->ty->name,
                  "alinhamento > 4 em variavel local nao suportado nesta "
                  "versao do kscc");

      bottom += var->ty->size;
      bottom = align_to(bottom, var->align);
      var->offset = -bottom;
    }

    // kscc: sp só precisa ser múltiplo de 4 (não 16 como no SysV).
    fn->stack_size = align_to(bottom, 4);
  }
}

// kscc: valor de 32 bits de uma relocação (`.word sym + addend`).
static void emit_reloc(Relocation *rel) {
  int32_t addend = (int32_t)(uint32_t)rel->addend;
  if (addend)
    println("  .word %s %c %d", *rel->label, addend < 0 ? '-' : '+',
            addend < 0 ? -addend : addend);
  else
    println("  .word %s", *rel->label);
}

static void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition)
      continue;

    if (var->is_tls)
      error_tok(var->ty->name, "_Thread_local nao suportado nesta versao do kscc");

    // kscc: o ksas não tem .local: um símbolo sem .global já é local ao
    // .ksx; a unicidade entre unidades vem do prefixo _S<id>_.
    if (!var->is_static)
      println("  .global %s", sym(var));

    // kscc: alinhamento natural máximo é 4; maiores (_Alignas) são
    // honrados porque o ksas alinha endereços absolutos.
    int align = var->align;

    // Common symbol: o ksas não tem .comm; opt_fcommon fica desligado
    // (ver main.c) e a definição provisória vira .bss.

    // .data / .rodata
    if (var->init_data) {
      println("  %s", var->is_str_lit ? ".rodata" : ".data");
      if (align > 1)
        println("  .align %d", align);
      println("%s:", sym(var));

      Relocation *rel = var->rel;
      int pos = 0;
      while (pos < var->ty->size) {
        if (rel && rel->offset == pos) {
          emit_reloc(rel);
          rel = rel->next;
          pos += 4;
          continue;
        }

        // Bytes em linhas de até 16, sem cruzar uma relocação.
        int end = pos;
        while (end < var->ty->size && end - pos < 16 && !(rel && rel->offset == end))
          end++;
        fprintf(output_file, "  .byte ");
        for (int i = pos; i < end; i++)
          fprintf(output_file, "%s%d", i == pos ? "" : ", ",
                  (unsigned char)var->init_data[i]);
        fprintf(output_file, "\n");
        pos = end;
      }
      continue;
    }

    // .bss
    println("  .bss");
    if (align > 1)
      println("  .align %d", align);
    println("%s:", sym(var));
    if (var->ty->size > 0)
      println("  .space %d", var->ty->size);
  }
}

// kscc: guarda um argumento recebido em registrador no seu slot local.
static void store_gp(int r, int offset, int sz) {
  char *base = "fp";
  if (!is_imm14(offset)) {
    add_imm("r5", "fp", offset);
    base = "r5";
    offset = 0;
  }

  switch (sz) {
  case 1:
    println("  stb [%s + %d], %s", base, offset, argreg[r]);
    return;
  case 2:
    println("  sth [%s + %d], %s", base, offset, argreg[r]);
    return;
  case 4:
    println("  stw [%s + %d], %s", base, offset, argreg[r]);
    return;
  }
  unreachable();
}

static void emit_text(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition)
      continue;

    // No code is emitted for "static inline" functions
    // if no one is referencing them.
    if (!fn->is_live)
      continue;

    Type *rty = fn->ty->return_ty;
    if (is_struct(rty))
      error_tok(fn->ty->name, "retorno de struct/union nao suportado nesta "
                "versao do kscc");
    if (is_flonum(rty) || (is_integer(rty) && rty->size > 4))
      error_tok(fn->ty->name, "tipo de retorno nao suportado nesta versao do kscc");

    fputc('\n', output_file);
    println("  .text");
    if (!fn->is_static)
      println("  .global %s", sym(fn));
    println("%s:", sym(fn));
    current_fn = fn;

    // Prologue (doc 02 §4)
    println("  push fp");
    println("  mov fp, sp");
    if (fn->stack_size > 0)
      add_imm("sp", "sp", -fn->stack_size);

    // Save passed-by-register arguments to the stack
    int gp = 0;
    for (Obj *var = fn->params; var; var = var->next) {
      if (var->offset > 0)
        continue;

      Type *ty = var->ty;
      if (is_flonum(ty) || ty->size > 4)
        error_tok(fn->ty->name, "tipo de parametro nao suportado nesta versao do kscc");
      store_gp(gp++, var->offset, ty->size);
    }

    // Emit code
    gen_stmt(fn->body);
    assert(depth == 0);

    // [https://www.sigbus.info/n1570#5.1.2.2.3p1] The C spec defines
    // a special rule for the main function. Reaching the end of the
    // main function is equivalent to returning 0, even though the
    // behavior is undefined for the other functions.
    if (strcmp(fn->name, "main") == 0)
      println("  li r1, 0");

    // Epilogue
    println("%s.return.%s:", L, fn->name);
    println("  mov sp, fp");
    println("  pop fp");
    println("  ret");
  }
}

// kscc: símbolos static recebem o prefixo _S<id>_ (doc 02 §5) para não
// colidir entre unidades concatenadas no link. A troca é feita no próprio
// Obj::name, de modo que as relocações (que guardam &var->name) acompanham.
// Uma declaração `extern` do mesmo nome na unidade (que o chibicc
// representa com outro Obj) refere-se ao mesmo objeto static e também é
// renomeada. Nomes anônimos (L<id>..N) já são únicos e não mudam.
static void mangle_static_symbols(Obj *prog) {
  HashMap statics = {0};

  for (Obj *var = prog; var; var = var->next)
    if (var->is_static && var->is_definition && !strchr(var->name, '.'))
      hashmap_put(&statics, var->name, (void *)1);

  for (Obj *var = prog; var; var = var->next)
    if (!strchr(var->name, '.') && hashmap_get(&statics, var->name)) {
      var->name = format("_S%s_%s", unit_id, var->name);
      var->is_static = true;
    }
}

void codegen(Obj *prog, FILE *out) {
  output_file = out;
  L = format("L%s", unit_id);

  // kscc: `.file` vira comentário no topo.
  println("; kscc: %s (unidade %s)", base_file, unit_id);
  File **files = get_input_files();
  for (int i = 0; files[i]; i++)
    println(";   arquivo %d: %s", files[i]->file_no, files[i]->name);
  println("  .text");

  mangle_static_symbols(prog);
  assign_lvar_offsets(prog);
  emit_data(prog);
  emit_text(prog);

  // kscc: termina em .text, estado neutro para a concatenação (doc 04 §3).
  println("  .text");
}
