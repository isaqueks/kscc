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
// Ponto flutuante (fase 6): float, double e long double são binary32 nos
// registradores inteiros, operados pelas instruções da extensão F da VM
// (doc 03 §1, float). Valor em r1 como qualquer escalar de 32 bits.
//
// Não suportado nesta versão (erro de compilação, nunca código errado):
// long long, TLS, atômicos e asm.

#include "chibicc.h"

// kscc: quatro registradores de argumento (doc 02 §3). Sem classe de ponto
// flutuante: um float passa como um int de 32 bits.
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

// kscc: escopos de labels para saltos para fora de `({ ... })` (ver
// unwind_to_label).
static HashMap label_scope;
static Node top_scope;

typedef struct {
  Node *scope;
  int depth;
} ScopeDepth;

static ScopeDepth *scope_depth;
static int scope_len, scope_cap;

// kscc: blocos com VLA (Node::vla_save): label_vla liga cada label ao
// bloco com VLA mais interno que o contém (top_scope se nenhum), e
// vla_blocks é a pilha dos que estão sendo gerados.
static HashMap label_vla;
static Node **vla_blocks;
static int vla_len, vla_cap;

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

// kscc: rd = [fp + off], com off de qualquer tamanho.
static void load_fp_slot(char *rd, int off) {
  if (is_imm14(off)) {
    println("  ldw %s, [fp + %d]", rd, off);
    return;
  }
  add_imm(rd, "fp", off);
  println("  ldw %s, [%s]", rd, rd);
}

// kscc: [fp + off] = rs; tmp é usado se off não cabe em 14 bits.
static void store_fp_slot(char *rs, int off, char *tmp) {
  if (is_imm14(off)) {
    println("  stw [fp + %d], %s", off, rs);
    return;
  }
  add_imm(tmp, "fp", off);
  println("  stw [%s], %s", tmp, rs);
}

// kscc: endereço de uma variável local em `reg`. Locais com alinhamento
// maior que 4 ficam numa área alinhada cuja base é calculada no prólogo e
// guardada num slot do frame (fp só é garantidamente múltiplo de 4).
static void lvar_addr(char *reg, Obj *var) {
  if (var->in_align_area) {
    load_fp_slot(reg, current_fn->align_base_offset);
    add_imm(reg, reg, var->offset);
    return;
  }
  add_imm(reg, "fp", var->offset);
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
    // kscc: o slot do VLA guarda o ponteiro devolvido por alloca.
    if (node->var->ty->kind == TY_VLA) {
      load_fp_slot("r1", node->var->offset);
      return;
    }

    // Local variable
    if (node->var->is_local) {
      lvar_addr("r1", node->var);
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
    add_imm("r1", "fp", node->var->offset);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

// kscc: r1 tem o campo nos bits altos (já deslocado para a esquerda);
// desloca de volta estendendo sinal ou zero. _Bool é sem sinal (um
// `_Bool b : 1` com 1 era lido como -1).
static void extend_bitfield(Member *mem) {
  if (mem->ty->is_unsigned || mem->ty->kind == TY_BOOL)
    println("  shri r1, r1, %d", 32 - mem->bit_width);
  else
    println("  sari r1, r1, %d", 32 - mem->bit_width);
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
  }

  // kscc: float/double/long double têm 4 bytes (ldw/stw, como int).
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
//
// Ponto flutuante: `fcmp r1, zr` (zr são os bits de +0.0). Z = 1 só para
// ±0 (igual), então beq/bne dão o `== 0` do C: -0 é falso, NaN
// (unordered, Z = 0) é verdadeiro. Todos os usuários de cmp_zero desviam
// por Z (beq/bne/set_cond eq/ne).
static void cmp_zero(Type *ty) {
  if (is_flonum(ty)) {
    println("  fcmp r1, zr");
    return;
  }
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

// kscc: as classes inteiras de até 32 bits mais F32 (float, double e long
// double, todos binary32; doc 03, cast). Ponteiros e enums são U32/I32.
enum { I8, I16, I32, U8, U16, U32, F32 };

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
  case TY_FLOAT:
  case TY_DOUBLE:
  case TY_LDOUBLE:
    return F32;
  }
  return U32;
}

// The table for type casts
static char i32i8[] = "sxb r1, r1";
static char i32u8[] = "zxb r1, r1";
static char i32i16[] = "sxh r1, r1";
static char i32u16[] = "zxh r1, r1";

// kscc: inteiro -> float: ITOF (com sinal, que serve para todos os
// valores já estendidos a 32 bits) ou UTOF (unsigned de 32 bits). float ->
// inteiro: FTOU para unsigned de 32 bits; FTOI para os demais, reduzido a
// char/short depois como numa conversão int -> char (o mesmo que o gcc faz
// com cvttss2si). Ambas truncam para zero e saturam; NaN dá 0 (fora do
// alcance é UB em C; o resultado da VM é definido e igual em todo host).
static char i32f32[] = "itof r1, r1";
static char u32f32[] = "utof r1, r1";
static char f32i8[] = "ftoi r1, r1\n  sxb r1, r1";
static char f32u8[] = "ftoi r1, r1\n  zxb r1, r1";
static char f32i16[] = "ftoi r1, r1\n  sxh r1, r1";
static char f32u16[] = "ftoi r1, r1\n  zxh r1, r1";
static char f32i32[] = "ftoi r1, r1";
static char f32u32[] = "ftou r1, r1";

static char *cast_table[][7] = {
  // i8   i16     i32     u8     u16     u32     f32
  {NULL,  NULL,   NULL,   i32u8, i32u16, NULL,   i32f32}, // i8
  {i32i8, NULL,   NULL,   i32u8, i32u16, NULL,   i32f32}, // i16
  {i32i8, i32i16, NULL,   i32u8, i32u16, NULL,   i32f32}, // i32
  {i32i8, NULL,   NULL,   NULL,  NULL,   NULL,   i32f32}, // u8
  {i32i8, i32i16, NULL,   i32u8, NULL,   NULL,   i32f32}, // u16
  {i32i8, i32i16, NULL,   i32u8, i32u16, NULL,   u32f32}, // u32
  {f32i8, f32i16, f32i32, f32u8, f32u16, f32u32, NULL},   // f32
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

// kscc: struct/union de N bytes como argumento: cópia na pilha,
// arredondada a múltiplo de 4 (doc 02 §3.1). r1 = endereço da origem.
// Os bytes de preenchimento ficam indefinidos, como no chibicc.
static void push_struct(Type *ty) {
  int sz = align_to(ty->size, 4);
  add_imm("sp", "sp", -sz);
  depth += sz / 4;

  // copy_bytes avança os registradores no laço: copia para r2, nunca sp.
  println("  mov r2, sp");
  copy_bytes("r1", "r2", ty->size);
}

static void push_args2(Node *args, bool first_pass) {
  if (!args)
    return;
  push_args2(args->next, first_pass);

  if ((first_pass && !args->pass_by_stack) || (!first_pass && args->pass_by_stack))
    return;

  gen_expr(args);

  switch (args->ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    push_struct(args->ty);
    break;
  default:
    push();
  }
}

// kscc: struct/union volta em r1 se tem até 4 bytes; senão o chamador
// passa um ponteiro oculto como primeiro argumento (doc 02 §3.3).
static bool has_ret_ptr(Node *node) {
  return node->ret_buffer && node->ty->size > 4;
}

// Load function call arguments.
//
// kscc: convenção da KSVM (doc 02 §3.1). Os argumentos são avaliados da
// direita para a esquerda e empilhados; os que vão em registrador são
// empilhados por último (segunda passada) para serem desempilhados em
// r1..r4 logo antes da chamada, deixando os da pilha no lugar, o mais à
// esquerda no endereço mais baixo.
//
// - Os 4 primeiros escalares vão em r1-r4; o resto, na pilha (4 bytes
//   cada).
// - Struct/union vai sempre na pilha, copiada inteira e arredondada a 4,
//   e não consome registrador.
// - Função variádica com protótipo: tudo na pilha (doc 02 §3.2).
// - Retorno de struct > 4 bytes: o endereço do buffer é um primeiro
//   argumento oculto (r1, ou o primeiro slot da pilha se variádica).
static int push_args(Node *node) {
  int stack = 0, gp = 0;
  bool variadic = is_abi_variadic(node->func_ty);

  if (has_ret_ptr(node)) {
    if (variadic)
      stack++;
    else
      gp++;
  }

  for (Node *arg = node->args; arg; arg = arg->next) {
    if (is_struct(arg->ty)) {
      arg->pass_by_stack = true;
      stack += align_to(arg->ty->size, 4) / 4;
      continue;
    }

    if (variadic || gp++ >= GP_MAX) {
      arg->pass_by_stack = true;
      stack++;
    }
  }

  push_args2(node->args, true);
  push_args2(node->args, false);

  // If the return type is a large struct/union, the caller passes
  // a pointer to a buffer as if it were the first argument.
  if (has_ret_ptr(node)) {
    lvar_addr("r1", node->ret_buffer);
    push();
  }

  return stack;
}

// kscc: depois da chamada, struct/union de até 4 bytes está em r1 (bytes
// baixos); copia para o buffer de retorno do chamador.
static void copy_ret_buffer(Obj *var) {
  int sz = var->ty->size;
  lvar_addr("r2", var);

  switch (sz) {
  case 0:
    return;
  case 1:
    println("  stb [r2], r1");
    return;
  case 2:
    println("  sth [r2], r1");
    return;
  case 3:
    println("  sth [r2], r1");
    println("  shri r1, r1, 16");
    println("  stb [r2 + 2], r1");
    return;
  case 4:
    println("  stw [r2], r1");
    return;
  }
  unreachable();
}

// kscc: `return` de struct/union de até 4 bytes: r1 = endereço do valor;
// carrega os bytes em r1 (os de preenchimento ficam zerados).
static void copy_struct_reg(void) {
  Type *ty = current_fn->ty->return_ty;

  switch (ty->size) {
  case 0:
    return;
  case 1:
    println("  ldb r1, [r1]");
    return;
  case 2:
    println("  ldh r1, [r1]");
    return;
  case 3:
    println("  ldh r2, [r1]");
    println("  ldb r1, [r1 + 2]");
    println("  shli r1, r1, 16");
    println("  or r1, r1, r2");
    return;
  case 4:
    println("  ldw r1, [r1]");
    return;
  }
  unreachable();
}

// kscc: `return` de struct/union maior: copia o valor (endereço em r1)
// para o buffer do chamador, recebido no parâmetro oculto, e devolve o
// endereço do buffer em r1 (doc 02 §3.3).
static void copy_struct_mem(void) {
  Type *ty = current_fn->ty->return_ty;
  Obj *var = current_fn->params;

  load_fp_slot("r2", var->offset);
  copy_bytes("r1", "r2", ty->size);
  load_fp_slot("r1", var->offset);
}

// kscc: alloca(n) com n em r1. Como no chibicc, os temporários que já
// estão na pilha (entre alloca_bottom e sp: argumentos empilhados,
// operandos pendentes) descem n bytes, e o bloco novo fica logo abaixo do
// alloca_bottom antigo. n é arredondado a 4 (doc 03 §2). A liberação é o
// `mov sp, fp` do epílogo.
static void builtin_alloca(void) {
  int c = count();
  int bottom = current_fn->alloca_bottom->offset;

  // Align size to 4 bytes.
  println("  addi r1, r1, 3");
  println("  andi r1, r1, -4");

  // Shift the temporary area by r1.
  load_fp_slot("r3", bottom);
  println("  sub r3, r3, sp");    // r3 = bytes de temporários (múltiplo de 4)
  println("  mov r4, sp");        // origem
  println("  sub sp, sp, r1");
  println("  mov r2, sp");        // destino
  println("%s.alloca.%d:", L, c);
  println("  cmp r3, zr");
  println("  beq %s.alloca_end.%d", L, c);
  println("  ldw r5, [r4]");
  println("  stw [r2], r5");
  println("  addi r4, r4, 4");
  println("  addi r2, r2, 4");
  println("  subi r3, r3, 4");
  println("  jmp %s.alloca.%d", L, c);
  println("%s.alloca_end.%d:", L, c);

  // Move alloca_bottom pointer.
  load_fp_slot("r2", bottom);
  println("  sub r2, r2, r1");
  store_fp_slot("r2", bottom, "r3");
  println("  mov r1, r2");
}

// kscc: alloca e __builtin_alloca são tratados pelo compilador.
static bool is_alloca_call(Node *node) {
  return node->lhs->kind == ND_VAR && node->lhs->var->ty->kind == TY_FUNC &&
         (!strcmp(node->lhs->var->name, "alloca") ||
          !strcmp(node->lhs->var->name, "__builtin_alloca"));
}

// kscc: __builtin_sqrtf e família (parse.c, float_builtin): uma instrução
// F, sem chamada. Os testes de classificação olham o binary32:
//   isnan:    FCMP x, x é unordered só para NaN (bvs)
//   isinf:    |x| == +inf
//   isfinite: |x| < +inf (NaN: unordered, falso)
//   signbit:  bit 31 (também em -0 e em NaN com sinal)
static void gen_fbuiltin(Node *node) {
  char *op = node->fop;

  if (node->rhs) {
    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("r2");
    println("  %s r1, r1, r2", op);
    return;
  }

  gen_expr(node->lhs);
  if (!strcmp(op, "isnan")) {
    println("  fcmp r1, r1");
    set_cond("vs");
  } else if (!strcmp(op, "isinf") || !strcmp(op, "isfinite")) {
    println("  fabs r1, r1");
    println("  li r2, %d ; inf", 0x7F800000);
    println("  fcmp r1, r2");
    set_cond(!strcmp(op, "isinf") ? "eq" : "mi");
  } else if (!strcmp(op, "signbit")) {
    println("  shri r1, r1, 31");
  } else {
    println("  %s r1, r1", op);
  }
}

// Generate code for a given node.
static void gen_expr(Node *node) {
  emit_loc(node->tok);
  check_type(node);

  switch (node->kind) {
  case ND_NULL_EXPR:
    return;
  case ND_NUM:
    // kscc: ponto flutuante: os bits do binary32, calculados no host (sem
    // depender do leitor de literais do ksas; `fli` seria o mesmo MOVIL).
    if (is_flonum(node->ty)) {
      float f = (float)node->fval;
      uint32_t bits;
      memcpy(&bits, &f, 4);
      println("  li r1, %d ; %.9g", (int32_t)bits, f);
      return;
    }
    // kscc: `li` é pseudo do ksas: MOVI se cabe em 14 bits, senão MOVIL.
    // O valor é reduzido a 32 bits (Node::val é int64_t).
    println("  li r1, %d", (int32_t)(uint32_t)node->val);
    return;
  case ND_NEG:
    gen_expr(node->lhs);
    // kscc: FNEG só troca o sinal (-0.0 a partir de 0.0, como o gcc).
    println("  %s r1, r1", is_flonum(node->ty) ? "fneg" : "neg");
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
      extend_bitfield(mem);
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
      // kscc: o valor da atribuição é o valor armazenado (C11 6.5.16p3),
      // truncado à largura do campo; o chibicc devolvia o lado direito.
      println("  mov r1, r6");
      println("  shli r1, r1, %d", 32 - mem->bit_width);
      extend_bitfield(mem);
      return;
    }

    store(node->ty);
    return;
  case ND_STMT_EXPR:
    if (scope_len == scope_cap) {
      scope_cap = scope_cap ? scope_cap * 2 : 8;
      scope_depth = realloc(scope_depth, sizeof(ScopeDepth) * scope_cap);
    }
    scope_depth[scope_len++] = (ScopeDepth){node, depth};
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    scope_len--;
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
    lvar_addr("r2", node->var);
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
    if (is_alloca_call(node)) {
      if (!current_fn->uses_alloca)
        unreachable();
      gen_expr(node->args);
      builtin_alloca();
      return;
    }

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

    // If the return type is a large struct/union, the caller passes
    // a pointer to a buffer as if it were the first argument.
    if (has_ret_ptr(node) && !is_abi_variadic(node->func_ty))
      pop(argreg[gp++]);

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

    // kscc: struct/union devolvida: o valor da expressão é o endereço do
    // buffer de retorno. Até 4 bytes, os bytes vêm em r1 e são copiados
    // para lá; acima, o chamado já escreveu no buffer (e devolveu o
    // endereço em r1, mas recalculá-lo não depende disso).
    if (node->ret_buffer) {
      if (!has_ret_ptr(node))
        copy_ret_buffer(node->ret_buffer);
      lvar_addr("r1", node->ret_buffer);
      return;
    }

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
  case ND_FBUILTIN:
    gen_fbuiltin(node);
    return;
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop("r2");

  // kscc: ponto flutuante (o parser converteu os dois lados para o mesmo
  // tipo): as quatro operações da extensão F e FCMP. FCMP segue o ARM VFP
  // (doc 04 §5.7 da ksvm2): == beq, != bne (verdadeiro com NaN), < bmi,
  // <= bls (falsos com NaN). `a > b` e `a >= b` chegam como `b < a` e
  // `b <= a` (o parser troca os lados), então também são falsos com NaN.
  // % e operadores de bits com float são erro em add_type.
  if (is_flonum(node->lhs->ty)) {
    switch (node->kind) {
    case ND_ADD:
      println("  fadd r1, r1, r2");
      return;
    case ND_SUB:
      println("  fsub r1, r1, r2");
      return;
    case ND_MUL:
      println("  fmul r1, r1, r2");
      return;
    case ND_DIV:
      // x / ±0 = ±inf (0/0 = NaN), sem fault.
      println("  fdiv r1, r1, r2");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  fcmp r1, r2");
      if (node->kind == ND_EQ)
        set_cond("eq");
      else if (node->kind == ND_NE)
        set_cond("ne");
      else if (node->kind == ND_LT)
        set_cond("mi");
      else
        set_cond("ls");
      return;
    }
    error_tok(node->tok, "invalid expression");
  }

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

// kscc: um salto (goto, break, continue) para fora de uma expressão-
// comando `({ ... })` pode partir com temporários na pilha: `x + ({ if (c)
// continue; 1; })` já empilhou x. O GCC permite sair assim (só é proibido
// entrar), e sem ajuste cada salto deixava 4 bytes por temporário na
// pilha (um laço estourava). Antes de gerar a função, label_scope liga
// cada label ao ND_STMT_EXPR mais interno que o contém (top_scope se
// nenhum); durante a geração, scope_depth guarda o `depth` em que cada
// ND_STMT_EXPR ativo começou, que é o `depth` dos seus labels. O salto
// desempilha a diferença. Fora de expressões-comando depth é 0 nos
// comandos, e nada muda.
//
// O mesmo percurso preenche label_vla, que diz de quais blocos com VLA um
// salto sai: os VLAs deles são liberados antes do salto
// (release_vla_to_label).
static void find_label_scopes(Node *node, Node *scope, Node *vla);

static void find_label_scopes_list(Node *node, Node *scope, Node *vla) {
  for (; node; node = node->next)
    find_label_scopes(node, scope, vla);
}

static void put_label_scope(char *label, Node *scope, Node *vla) {
  if (!label)
    return;
  hashmap_put(&label_scope, label, scope);
  hashmap_put(&label_vla, label, vla);
}

static void find_label_scopes(Node *node, Node *scope, Node *vla) {
  if (!node)
    return;

  switch (node->kind) {
  case ND_LABEL:
    put_label_scope(node->unique_label, scope, vla);
    break;
  case ND_FOR:
  case ND_DO:
  case ND_SWITCH:
    put_label_scope(node->brk_label, scope, vla);
    put_label_scope(node->cont_label, scope, vla);
    break;
  case ND_STMT_EXPR:
    find_label_scopes_list(node->body, node, vla);
    return;
  case ND_BLOCK:
    if (node->vla_save) {
      find_label_scopes_list(node->body, scope, node);
      return;
    }
    break;
  }

  find_label_scopes(node->lhs, scope, vla);
  find_label_scopes(node->rhs, scope, vla);
  find_label_scopes(node->cond, scope, vla);
  find_label_scopes(node->then, scope, vla);
  find_label_scopes(node->els, scope, vla);
  find_label_scopes(node->init, scope, vla);
  find_label_scopes(node->inc, scope, vla);
  find_label_scopes_list(node->body, scope, vla);
  find_label_scopes_list(node->args, scope, vla);
}

// Desempilha os temporários acima do nível do label antes de saltar.
// Devolve o `depth` com que se chega ao label.
static int unwind_to_label(char *label) {
  if (depth == 0)
    return 0;

  Node *scope = hashmap_get(&label_scope, label);
  if (!scope)
    return depth;

  int target = -1;
  if (scope == &top_scope)
    target = 0;
  for (int i = scope_len - 1; i >= 0 && target < 0; i--)
    if (scope_depth[i].scope == scope)
      target = scope_depth[i].depth;

  // Alvo dentro de uma expressão-comando que não está ativa: salto para
  // dentro dela, que o C não permite. Nada a ajustar.
  if (target < 0 || target >= depth)
    return depth;
  add_imm("sp", "sp", (depth - target) * 4);
  return target;
}

// kscc: libera os VLAs de um bloco: alloca_bottom e sp voltam ao valor
// guardado na entrada (Node::vla_save). Os `ntemps` temporários que
// continuam vivos (os do nível do comando) ficam entre sp e alloca_bottom
// e sobem junto; o destino está acima da origem, então a cópia vai de
// cima para baixo. Destrói r3 e r4.
static void vla_restore(Obj *save, int ntemps) {
  load_fp_slot("r3", save->offset);
  store_fp_slot("r3", current_fn->alloca_bottom->offset, "r4");
  if (ntemps > 0) {
    add_imm("r3", "r3", -ntemps * 4);
    for (int i = ntemps - 1; i >= 0; i--) {
      println("  ldw r4, [sp + %d]", i * 4);
      println("  stw [r3 + %d], r4", i * 4);
    }
  }
  println("  mov sp, r3");
}

// kscc: antes de um salto, libera os VLAs dos blocos de que ele sai: basta
// restaurar o mais externo deles. `ntemps` é o depth no label.
static void release_vla_to_label(char *label, int ntemps) {
  if (vla_len == 0)
    return;

  Node *target = hashmap_get(&label_vla, label);
  if (!target)
    return;

  int i = 0;
  if (target != &top_scope) {
    for (i = vla_len - 1; i >= 0 && vla_blocks[i] != target; i--)
      ;
    // Alvo num bloco com VLA que não está ativo: salto para dentro do
    // escopo de um VLA, que o C não permite.
    if (i < 0)
      return;
    i++;
  }
  if (i < vla_len)
    vla_restore(vla_blocks[i]->vla_save, ntemps);
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
      add_imm("r2", "r1", (int32_t)(0u - (uint32_t)begin));
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
    // kscc: bloco que declara VLA: guarda alloca_bottom na entrada e o
    // restaura na saída (e nos saltos para fora, release_vla_to_label),
    // liberando o VLA no fim do seu escopo (C11 6.2.4p7).
    if (node->vla_save) {
      load_fp_slot("r1", current_fn->alloca_bottom->offset);
      store_fp_slot("r1", node->vla_save->offset, "r2");
      if (vla_len == vla_cap) {
        vla_cap = vla_cap ? vla_cap * 2 : 8;
        vla_blocks = realloc(vla_blocks, sizeof(Node *) * vla_cap);
      }
      vla_blocks[vla_len++] = node;
    }
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    if (node->vla_save) {
      vla_len--;
      vla_restore(node->vla_save, depth);
    }
    return;
  case ND_GOTO: {
    int ntemps = unwind_to_label(node->unique_label);
    release_vla_to_label(node->unique_label, ntemps);
    println("  jmp %s", node->unique_label);
    return;
  }
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
      gen_expr(node->lhs);
      Type *ty = node->lhs->ty;

      switch (ty->kind) {
      case TY_STRUCT:
      case TY_UNION:
        if (ty->size <= 4)
          copy_struct_reg();
        else
          copy_struct_mem();
        break;
      }
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
// de retorno, argumentos na pilha a partir de fp+8, um por palavra
// (struct/union: o tamanho arredondado a 4). Os que chegam em r1-r4 são
// copiados para slots locais no prólogo. A ordem de classificação é a
// mesma de push_args: o ponteiro oculto de retorno de struct (primeiro
// parâmetro, criado pelo parser) conta como um escalar.
//
// Locais com alinhamento > 4 (_Alignas, structs com membros alinhados):
// fp só é garantidamente múltiplo de 4, então elas vão para uma área
// separada no fundo do frame, endereçada a partir de uma base alinhada
// calculada no prólogo, B = (fp - align_top) & -align_max, e guardada no
// slot fp + align_base_offset. O frame reserva align_max - 4 bytes de
// folga para o arredondamento. Dentro da área o layout é o do chibicc, de
// modo que `_Alignas(32) char x, y;` dá &y - &x == 32.
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
      if (is_struct(var->ty)) {
        // kscc: a cópia do chamador está só alinhada a 4.
        if (var->align > 4)
          error_tok(fn->ty->name, "parametro struct/union com alinhamento > 4 "
                    "nao suportado nesta versao do kscc");
        var->offset = top;
        top += align_to(var->ty->size, 4);
        continue;
      }

      if (!variadic && gp++ < GP_MAX)
        continue;

      var->offset = top;
      top += 4;
    }

    // Assign offsets to pass-by-register parameters and local variables.
    int abottom = 0;
    int amax = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      if (var->offset)
        continue;

      if (var->align > 4) {
        abottom += var->ty->size;
        abottom = align_to(abottom, var->align);
        var->offset = -abottom;
        var->in_align_area = true;
        amax = MAX(amax, var->align);
        continue;
      }

      bottom += var->ty->size;
      bottom = align_to(bottom, var->align);
      var->offset = -bottom;
    }

    // kscc: sp só precisa ser múltiplo de 4 (não 16 como no SysV).
    bottom = align_to(bottom, 4);

    if (amax) {
      bottom += 4;
      fn->align_base_offset = -bottom;
      fn->align_top = bottom;
      fn->align_max = amax;
      bottom += amax - 4 + align_to(abottom, amax);
    }

    fn->stack_size = bottom;
  }
}

// kscc: valor de 32 bits de uma relocação (`.word sym + addend`).
static void emit_reloc(Relocation *rel) {
  int32_t addend = (int32_t)(uint32_t)rel->addend;
  // A magnitude é calculada sem sinal: -INT32_MIN não cabe em int.
  uint32_t mag = addend < 0 ? 0u - (uint32_t)addend : (uint32_t)addend;
  if (addend)
    println("  .word %s %c %u", *rel->label, addend < 0 ? '-' : '+', mag);
  else
    println("  .word %s", *rel->label);
}

// kscc: objeto const (ou array de const): vai para .rodata, que fica na
// região de código, só leitura. Escrever nele dá FAULT_MEM_PERM em vez de
// corromper o valor (doc 02 §5).
static bool is_const_object(Type *ty) {
  for (; ty->kind == TY_ARRAY; ty = ty->base)
    if (ty->is_const)
      return true;
  return ty->is_const;
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

    // Common symbol
    //
    // kscc: o ksas não tem .comm. A definição provisória (`int x;` sem
    // static/extern/inicializador) vira uma linha-marcador que o link
    // (src/link.c, resolve_commons) junta entre as unidades: se alguma
    // unidade define o símbolo, vale essa definição; senão o link cria um
    // único .bss com o maior tamanho e alinhamento pedidos.
    if (opt_fcommon && var->is_tentative && !var->is_static) {
      println("; kscc-common: %s %d %d", sym(var), MAX(var->ty->size, 0), align);
      continue;
    }

    // .data / .rodata
    if (var->init_data) {
      bool rodata = var->is_str_lit || is_const_object(var->ty);
      println("  %s", rodata ? ".rodata" : ".data");
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
    if (is_integer(rty) && rty->size > 4)
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

    // kscc: só funções que usam alloca/VLA precisam do alloca_bottom.
    if (fn->uses_alloca)
      store_fp_slot("sp", fn->alloca_bottom->offset, "r5");

    // kscc: base da área de locais com alinhamento > 4.
    if (fn->align_max) {
      add_imm("r5", "fp", -fn->align_top);
      if (is_imm14(-fn->align_max))
        println("  andi r5, r5, %d", -fn->align_max);
      else
        println("  andl r5, r5, %d", -fn->align_max);
      store_fp_slot("r5", fn->align_base_offset, "r6");
    }

    // Save passed-by-register arguments to the stack
    int gp = 0;
    for (Obj *var = fn->params; var; var = var->next) {
      if (var->offset > 0)
        continue;

      Type *ty = var->ty;
      if (ty->size > 4)
        error_tok(fn->ty->name, "tipo de parametro nao suportado nesta versao do kscc");
      store_gp(gp++, var->offset, ty->size);
    }

    // Emit code
    label_scope = (HashMap){0};
    label_vla = (HashMap){0};
    find_label_scopes(fn->body, &top_scope, &top_scope);
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

// kscc: o ksas só aceita identificadores [A-Za-z_][A-Za-z0-9_.]*, mas C
// aceita UCNs e UTF-8 em nomes (`int π;`) e o chibicc também '$'. Cada
// byte fora de [A-Za-z0-9_] vira ".xx" (hex), com "_" na frente se o
// primeiro caractere for escapado: `π` -> `_.cf.80`. Como nenhum nome C
// contém '.', o resultado não colide com outros símbolos, e todas as
// unidades escapam o mesmo nome do mesmo jeito. Roda depois de
// mangle_static_symbols (que trata nomes com '.' como anônimos).
static bool is_asm_ident_byte(unsigned char c) {
  return isalnum(c) || c == '_';
}

// kscc: o ksas lê como registrador, sem diferenciar maiúsculas, zr, fp,
// sp, r0-r31 e os apelidos a0-a3, t0-t10, s0-s13 (ksas_reg_parse); um
// global C com esse nome (`int fp;`, `int s1;`) não pode virar label.
// Esses nomes ganham um '.' no fim (`fp` -> `fp.`), que o escape em
// hexadecimal nunca produz (ele sempre põe dois dígitos depois do '.').
static bool is_ksas_register(char *name) {
  char lc[4];
  int n = strlen(name);
  if (n < 2 || n > 3)
    return false;
  for (int i = 0; i <= n; i++)
    lc[i] = tolower((unsigned char)name[i]);

  if (!strcmp(lc, "zr") || !strcmp(lc, "fp") || !strcmp(lc, "sp"))
    return true;

  // Uma ou duas casas decimais, sem zero à esquerda ("r01" não é).
  if (!isdigit((unsigned char)lc[1]) ||
      (n == 3 && (!isdigit((unsigned char)lc[2]) || lc[1] == '0')))
    return false;

  int v = atoi(lc + 1);
  switch (lc[0]) {
  case 'r': return v < 32;
  case 'a': return v < 4;
  case 't': return v < 11;
  case 's': return v < 14;
  }
  return false;
}

static void escape_symbol_names(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    char *name = var->name;
    if (strchr(name, '.'))
      continue; // anônimo (L<id>..N), já válido

    if (is_ksas_register(name)) {
      var->name = format("%s.", name);
      continue;
    }

    bool ok = true;
    for (char *p = name; *p; p++)
      if (!is_asm_ident_byte(*p))
        ok = false;
    if (ok)
      continue;

    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);
    if (!is_asm_ident_byte(*name) || isdigit((unsigned char)*name))
      fputc('_', out);
    for (unsigned char *p = (unsigned char *)name; *p; p++) {
      if (is_asm_ident_byte(*p))
        fputc(*p, out);
      else
        fprintf(out, ".%02x", *p);
    }
    fclose(out);
    var->name = buf;
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
  escape_symbol_names(prog);
  assign_lvar_offsets(prog);
  emit_data(prog);
  emit_text(prog);

  // kscc: termina em .text, estado neutro para a concatenação (doc 04 §3).
  println("  .text");
}
