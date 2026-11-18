#include "chibicc.h"

Type *ty_void = &(Type){.kind = TY_VOID, .size = 1, .align = 1};
Type *ty_bool = &(Type){.kind = TY_BOOL, .size = 1, .align = 1};

Type *ty_char = &(Type){.kind = TY_CHAR, .size = 1, .align = 1};
Type *ty_short = &(Type){.kind = TY_SHORT, .size = 2, .align = 2};
Type *ty_int = &(Type){.kind = TY_INT, .size = 4, .align = 4};
// kscc: ILP32 (doc 02 §1): long e ponteiros têm 4 bytes.
Type *ty_long = &(Type){.kind = TY_LONG, .size = 4, .align = 4};

Type *ty_uchar = &(Type){.kind = TY_CHAR, .size = 1, .align = 1, .is_unsigned = true};
Type *ty_ushort = &(Type){.kind = TY_SHORT, .size = 2, .align = 2, .is_unsigned = true};
Type *ty_uint = &(Type){.kind = TY_INT, .size = 4, .align = 4, .is_unsigned = true};
Type *ty_ulong = &(Type){.kind = TY_LONG, .size = 4, .align = 4, .is_unsigned = true};

// kscc: tipos de 64 bits que só aparecem em #if (literais grandes avaliados
// em intmax_t). Fora do preprocessador, o parser rejeita (fase 5).
Type *ty_llong = &(Type){.kind = TY_LONG, .size = 8, .align = 4};
Type *ty_ullong = &(Type){.kind = TY_LONG, .size = 8, .align = 4, .is_unsigned = true};

// kscc: a KSVM só tem binary32 (extensão F, doc 04 §5.7 da ksvm2), então
// float, double e long double têm todos 4 bytes, alinhamento 4 e a mesma
// representação (divergência deliberada da norma, como o -fshort-double do
// avr-gcc antigo; doc 02 §1). Os três kinds continuam distintos para a
// semântica da linguagem (_Generic, compatibilidade de tipos, promoções).
Type *ty_float = &(Type){.kind = TY_FLOAT, .size = 4, .align = 4};
Type *ty_double = &(Type){.kind = TY_DOUBLE, .size = 4, .align = 4};
Type *ty_ldouble = &(Type){.kind = TY_LDOUBLE, .size = 4, .align = 4};

static Type *new_type(TypeKind kind, int size, int align) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  ty->size = size;
  ty->align = align;
  return ty;
}

bool is_integer(Type *ty) {
  TypeKind k = ty->kind;
  return k == TY_BOOL || k == TY_CHAR || k == TY_SHORT ||
         k == TY_INT  || k == TY_LONG || k == TY_ENUM;
}

bool is_flonum(Type *ty) {
  return ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE ||
         ty->kind == TY_LDOUBLE;
}

bool is_numeric(Type *ty) {
  return is_integer(ty) || is_flonum(ty);
}

bool is_compatible(Type *t1, Type *t2) {
  if (t1 == t2)
    return true;

  if (t1->origin)
    return is_compatible(t1->origin, t2);

  if (t2->origin)
    return is_compatible(t1, t2->origin);

  if (t1->kind != t2->kind)
    return false;

  switch (t1->kind) {
  case TY_CHAR:
  case TY_SHORT:
  case TY_INT:
  case TY_LONG:
    return t1->is_unsigned == t2->is_unsigned;
  case TY_FLOAT:
  case TY_DOUBLE:
  case TY_LDOUBLE:
    return true;
  case TY_PTR:
    return is_compatible(t1->base, t2->base);
  case TY_FUNC: {
    if (!is_compatible(t1->return_ty, t2->return_ty))
      return false;
    if (t1->is_variadic != t2->is_variadic)
      return false;

    Type *p1 = t1->params;
    Type *p2 = t2->params;
    for (; p1 && p2; p1 = p1->next, p2 = p2->next)
      if (!is_compatible(p1, p2))
        return false;
    return p1 == NULL && p2 == NULL;
  }
  case TY_ARRAY:
    if (!is_compatible(t1->base, t2->base))
      return false;
    return t1->array_len < 0 && t2->array_len < 0 &&
           t1->array_len == t2->array_len;
  }
  return false;
}

Type *copy_type(Type *ty) {
  Type *ret = calloc(1, sizeof(Type));
  *ret = *ty;
  ret->origin = ty;
  return ret;
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR, 4, 4); // kscc: ILP32
  ty->base = base;
  ty->is_unsigned = true;
  return ty;
}

Type *func_type(Type *return_ty) {
  // The C spec disallows sizeof(<function type>), but
  // GCC allows that and the expression is evaluated to 1.
  Type *ty = new_type(TY_FUNC, 1, 1);
  ty->return_ty = return_ty;
  return ty;
}

Type *array_of(Type *base, int len) {
  Type *ty = new_type(TY_ARRAY, base->size * len, base->align);
  ty->base = base;
  ty->array_len = len;
  return ty;
}

Type *vla_of(Type *base, Node *len) {
  Type *ty = new_type(TY_VLA, 4, 4); // kscc: ILP32
  ty->base = base;
  ty->vla_len = len;
  return ty;
}

Type *enum_type(void) {
  return new_type(TY_ENUM, 4, 4);
}

Type *struct_type(void) {
  return new_type(TY_STRUCT, 0, 1);
}

// kscc: posto de conversão (C11 6.3.1.1) dos tipos inteiros de 4 bytes.
static int int_rank(Type *ty) {
  return ty->kind == TY_LONG ? 2 : 1;
}

static Type *get_common_type(Type *ty1, Type *ty2) {
  if (ty1->base)
    return pointer_to(ty1->base);

  if (ty1->kind == TY_FUNC)
    return pointer_to(ty1);
  if (ty2->kind == TY_FUNC)
    return pointer_to(ty2);

  if (ty1->kind == TY_LDOUBLE || ty2->kind == TY_LDOUBLE)
    return ty_ldouble;
  if (ty1->kind == TY_DOUBLE || ty2->kind == TY_DOUBLE)
    return ty_double;
  if (ty1->kind == TY_FLOAT || ty2->kind == TY_FLOAT)
    return ty_float;

  if (ty1->size < 4)
    ty1 = ty_int;
  if (ty2->size < 4)
    ty2 = ty_int;

  if (ty1->size != ty2->size)
    return (ty1->size < ty2->size) ? ty2 : ty1;

  // kscc: na ILP32 int e long têm o mesmo tamanho, então a regra do C11
  // 6.3.1.8 precisa olhar o posto (long > int), não só o tamanho:
  // long + unsigned int dá unsigned long (o long não representa todos os
  // valores do unsigned int).
  if (ty1->is_unsigned == ty2->is_unsigned)
    return (int_rank(ty2) > int_rank(ty1)) ? ty2 : ty1;

  Type *u = ty1->is_unsigned ? ty1 : ty2;
  Type *s = ty1->is_unsigned ? ty2 : ty1;
  if (int_rank(u) >= int_rank(s))
    return u;
  return (s->kind == TY_LONG) ? ty_ulong : ty_uint;
}

// For many binary operators, we implicitly promote operands so that
// both operands have the same type. Any integral type smaller than
// int is always promoted to int. If the type of one operand is larger
// than the other's (e.g. "long" vs. "int"), the smaller operand will
// be promoted to match with the other.
//
// This operation is called the "usual arithmetic conversion".
// kscc: promoção inteira de bitfield (C11 6.3.1.1p2): um bitfield de
// largura < 32 cabe em int e promove a int mesmo quando declarado unsigned
// (`unsigned u : 12` → int). O chibicc usava o tipo declarado.
// Vale também para o resultado de uma atribuição ao campo (`(s.u = x)`,
// `++s.u`, `s.u += x`, que o parser transforma em vírgula + atribuição).
static Node *promote_bitfield(Node *node) {
  Node *n = node;
  while (n->kind == ND_COMMA)
    n = n->rhs;
  if (n->kind == ND_ASSIGN)
    n = n->lhs;
  if (n->kind == ND_MEMBER && n->member->is_bitfield &&
      n->member->bit_width < 32 && is_integer(node->ty))
    return new_cast(node, ty_int);
  return node;
}

static void usual_arith_conv(Node **lhs, Node **rhs) {
  *lhs = promote_bitfield(*lhs);
  *rhs = promote_bitfield(*rhs);
  Type *ty = get_common_type((*lhs)->ty, (*rhs)->ty);
  *lhs = new_cast(*lhs, ty);
  *rhs = new_cast(*rhs, ty);
}

void add_type(Node *node) {
  if (!node || node->ty)
    return;

  add_type(node->lhs);
  add_type(node->rhs);
  add_type(node->cond);
  add_type(node->then);
  add_type(node->els);
  add_type(node->init);
  add_type(node->inc);

  for (Node *n = node->body; n; n = n->next)
    add_type(n);
  for (Node *n = node->args; n; n = n->next)
    add_type(n);

  switch (node->kind) {
  case ND_NUM:
    node->ty = ty_int;
    return;
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_MOD:
  case ND_BITAND:
  case ND_BITOR:
  case ND_BITXOR:
    usual_arith_conv(&node->lhs, &node->rhs);
    node->ty = node->lhs->ty;
    // kscc: % e os operadores de bits exigem inteiros (C11 6.5.5p2,
    // 6.5.10-12); o chibicc só acusava no codegen.
    if (node->kind != ND_ADD && node->kind != ND_SUB && node->kind != ND_MUL &&
        node->kind != ND_DIV && is_flonum(node->ty))
      error_tok(node->tok, "operando de ponto flutuante invalido para este operador");
    return;
  case ND_NEG: {
    node->lhs = promote_bitfield(node->lhs);
    Type *ty = get_common_type(ty_int, node->lhs->ty);
    node->lhs = new_cast(node->lhs, ty);
    node->ty = ty;
    return;
  }
  case ND_ASSIGN:
    if (node->lhs->ty->kind == TY_ARRAY)
      error_tok(node->lhs->tok, "not an lvalue");
    if (node->lhs->ty->kind != TY_STRUCT)
      node->rhs = new_cast(node->rhs, node->lhs->ty);
    node->ty = node->lhs->ty;
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    usual_arith_conv(&node->lhs, &node->rhs);
    node->ty = ty_int;
    return;
  case ND_FUNCALL:
    node->ty = node->func_ty->return_ty;
    return;
  case ND_NOT:
  case ND_LOGOR:
  case ND_LOGAND:
    node->ty = ty_int;
    return;
  case ND_BITNOT:
  case ND_SHL:
  case ND_SHR:
    // kscc: promoção inteira do operando (C11 6.5.7p3, 6.5.3.3p4), que o
    // chibicc omitia. Com valores reduzidos ao tipo (eval e casts),
    // `(char)1 << 10` precisa ter tipo int.
    node->lhs = promote_bitfield(node->lhs);
    if (is_flonum(node->lhs->ty) || (node->rhs && is_flonum(node->rhs->ty)))
      error_tok(node->tok, "operando de ponto flutuante invalido para este operador");
    if (is_integer(node->lhs->ty) && node->lhs->ty->size < 4)
      node->lhs = new_cast(node->lhs, ty_int);
    node->ty = node->lhs->ty;
    return;
  case ND_VAR:
  case ND_VLA_PTR:
    node->ty = node->var->ty;
    return;
  case ND_COND:
    if (node->then->ty->kind == TY_VOID || node->els->ty->kind == TY_VOID) {
      node->ty = ty_void;
    } else if ((node->then->ty->kind == TY_STRUCT || node->then->ty->kind == TY_UNION) &&
               node->then->ty->kind == node->els->ty->kind) {
      // kscc: struct/union não passa pelas conversões aritméticas: com
      // menos de 4 bytes, a promoção a int fazia do valor o endereço da
      // struct com tipo int (herdado do chibicc).
      node->ty = node->then->ty;
    } else {
      usual_arith_conv(&node->then, &node->els);
      node->ty = node->then->ty;
    }
    return;
  case ND_COMMA:
    node->ty = node->rhs->ty;
    return;
  case ND_MEMBER:
    node->ty = node->member->ty;
    return;
  case ND_ADDR:
    // kscc: &a, com a array de T[n], é um ponteiro para o array, T (*)[n]
    // (C11 6.5.3.2p3); o chibicc dava T *, e `&a + 1` avançava sizeof(T)
    // em vez de sizeof(a).
    node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF:
    if (!node->lhs->ty->base)
      error_tok(node->tok, "invalid pointer dereference");
    if (node->lhs->ty->base->kind == TY_VOID)
      error_tok(node->tok, "dereferencing a void pointer");

    node->ty = node->lhs->ty->base;
    return;
  case ND_STMT_EXPR:
    if (node->body) {
      Node *stmt = node->body;
      while (stmt->next)
        stmt = stmt->next;
      if (stmt->kind == ND_EXPR_STMT) {
        node->ty = stmt->lhs->ty;
        return;
      }
    }
    error_tok(node->tok, "statement expression returning void is not supported");
    return;
  case ND_LABEL_VAL:
    node->ty = pointer_to(ty_void);
    return;
  case ND_CAS:
    add_type(node->cas_addr);
    add_type(node->cas_old);
    add_type(node->cas_new);
    node->ty = ty_bool;

    if (node->cas_addr->ty->kind != TY_PTR)
      error_tok(node->cas_addr->tok, "pointer expected");
    if (node->cas_old->ty->kind != TY_PTR)
      error_tok(node->cas_old->tok, "pointer expected");
    return;
  case ND_EXCH:
    if (node->lhs->ty->kind != TY_PTR)
      error_tok(node->cas_addr->tok, "pointer expected");
    node->ty = node->lhs->ty->base;
    return;
  }
}
