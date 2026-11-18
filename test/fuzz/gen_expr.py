#!/usr/bin/env python3
"""gen_expr.py — gerador de programas C aleatórios sem UB para o fuzz
diferencial kscc x gcc -m32 (test/fuzz/run_fuzz.sh).

    python3 test/fuzz/gen_expr.py SEMENTE > prog.c

O mesmo número de semente gera sempre o mesmo programa. O programa usa só
test/kstest.h para imprimir e termina com exit 0.

Como evitar UB (C11) sem perder cobertura de operações com sinal:
  * aritmética int + int / int - int:   ((a) >> 1) + ((b) >> 1)   (cabe em 32 bits)
  * int * int:                          (short)(a) * (short)(b)   (|produto| <= 2^30)
  * -int:                               -((a) >> 1)
  * divisão e resto:                    divisor ((b) & 7) | 1  (int, 1..7) ou
                                        (b) | 1u (unsigned): nunca 0 nem -1
  * shift:                              contagem (b) & 31; << com sinal é feito em
                                        unsigned e convertido de volta (definido pela
                                        implementação, igual nos dois alvos)
  * qualquer operação em que um operando é unsigned é feita em unsigned (wrap)
  * atribuição composta: rhs convertido para unsigned (v op= (unsigned)(e)),
    <<= só em variáveis unsigned de 32 bits
  * ++/-- só em variáveis unsigned ou menores que int
  * índices de array mascarados: a[(e) & 7]
  * structs por valor: struct T copiada, passada e devolvida por mix() (pura),
    literais compostos; ponteiros para escalares (parâmetros T *p usados
    como (*p)) e para arrays globais
  * chamadas só em comandos (v = f(...); com v escalar, sem índice), nunca
    dentro de expressões (a
    ordem de avaliação dos operandos não é especificada e as funções
    escrevem em globais); uma função só chama as anteriores (sem recursão)
  * laços com contador que o corpo não altera; o custo de cada função é
    estimado para limitar o total de passos
Conversões para tipos menores com sinal são definidas pela implementação
(módulo 2^n no gcc e no kscc) e fazem parte do que é testado.

    python3 test/fuzz/gen_expr.py SEMENTE float > prog.c

gera um programa de ponto flutuante (FloatGen): só float (nunca double),
comparado com o gcc -m32 -msse -mfpmath=sse -fsingle-precision-constant
(binary32 sem precisão estendida, como a KSVM). Sem UB nem diferença
legítima entre os dois:
  * float -> inteiro só por f2i/f2u, que devolvem uma constante fora do
    alcance e para NaN (a conversão seria UB; o kscc satura, o x86 não)
  * divisão por zero, inf e NaN à vontade (IEEE, definidos), mas todo
    float impresso ou misturado ao hash passa por fb(), que troca qualquer
    NaN pelo canônico (o x86 produz 0xFFC00000, a KSVM 0x7FC00000)
  * sem fminf/fmaxf (fmax(-0, +0) não é fixado pelo C)
  * funções float são puras (só leem globais), então podem aparecer
    dentro de expressões sem depender da ordem de avaliação
  * contadores de laço inteiros; índices mascarados
"""

import random
import sys

TYPES = ["int", "unsigned", "char", "signed char", "unsigned char", "short", "unsigned short"]
UNSIGNED = {"unsigned", "unsigned char", "unsigned short"}
ARR_LEN = 8
STRUCT_FIELDS = [("a", "char"), ("b", "short"), ("c", "int"), ("d", "unsigned"), ("e", "unsigned char")]
REF_TYPES = [ty for _, ty in STRUCT_FIELDS]
MAX_FUNC_COST = 3000


def kind(ty):
    """Tipo após a promoção inteira: 'u' (unsigned) ou 'i' (int)."""
    return "u" if ty == "unsigned" else "i"


class Var:
    def __init__(self, name, ty, is_array=False, writable=True):
        self.name, self.ty, self.is_array, self.writable = name, ty, is_array, writable


class Gen:
    def __init__(self, seed):
        self.r = random.Random(seed)
        self.seed = seed
        self.out = []
        self.globals = []
        self.funcs = []  # (nome, tipo de retorno, [(tipo, nome, é_ponteiro)], custo)
        self.ind = 0
        self.uid = 0

    # ---------------------------------------------------------------- util
    def emit(self, line):
        self.out.append("  " * self.ind + line)

    def fresh(self, prefix):
        self.uid += 1
        return "%s%d" % (prefix, self.uid)

    def const(self, want=None):
        r = self.r
        k = want or r.choice("iiiu")
        c = r.random()
        if k == "u":
            v = r.choice([0, 1, 2, 7, 255, 256, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF,
                          r.randrange(0, 1 << 32), r.randrange(0, 300)])
            return ("0x%Xu" % v if c < 0.5 else "%du" % v), "u"
        if c < 0.15:
            return "(-2147483647 - 1)", "i"
        if c < 0.3:
            return str(r.choice([0, 1, -1, 2, 127, -128, 255, 32767, -32768, 65535, 2147483647])), "i"
        if c < 0.4:
            return "'%s'" % r.choice(["a", "Z", "0", "\\xff", "\\x80", "\\n", " "]), "i"
        if c < 0.75:
            v = r.randrange(-1000, 1000)
            return ("(%d)" % v if v < 0 else str(v)), "i"
        v = r.randrange(-(1 << 31) + 1, 1 << 31)
        return ("(%d)" % v if v < 0 else str(v)), "i"

    # ------------------------------------------------------------ expressões
    def lvalue(self, scope, writable=True):
        cands = [v for v in scope if (v.writable or not writable)]
        v = self.r.choice(cands)
        if v.is_array:
            idx, _ = self.expr(scope, 1)
            return "%s[(%s) & %d]" % (v.name, idx, ARR_LEN - 1), v.ty
        return v.name, v.ty

    def leaf(self, scope):
        if self.r.random() < 0.35 or not scope:
            return self.const()
        name, ty = self.lvalue(scope, writable=False)
        return name, kind(ty)

    def expr(self, scope, depth):
        r = self.r
        if depth <= 0 or r.random() < 0.2:
            return self.leaf(scope)
        c = r.random()
        if c < 0.45:
            return self.binary(scope, depth)
        if c < 0.55:
            a, ka = self.expr(scope, depth - 1)
            op = r.choice(["~", "!", "-", "+"])
            if op == "~":
                return "(~(%s))" % a, ka
            if op == "!":
                return "(!(%s))" % a, "i"
            if op == "-":
                return ("(-((%s) >> 1))" % a if ka == "i" else "(-(%s))" % a), ka
            return "(+(%s))" % a, ka
        if c < 0.65:
            ty = r.choice(TYPES)
            a, _ = self.expr(scope, depth - 1)
            return "((%s)(%s))" % (ty, a), kind(ty)
        if c < 0.75:
            cnd, _ = self.expr(scope, depth - 1)
            a, ka = self.expr(scope, depth - 1)
            b, kb = self.expr(scope, depth - 1)
            k = "u" if "u" in (ka, kb) else "i"
            return "((%s) ? (%s) : (%s))" % (cnd, a, b), k
        if c < 0.85:
            a, _ = self.expr(scope, depth - 1)
            b, _ = self.expr(scope, depth - 1)
            op = r.choice(["<", "<=", ">", ">=", "==", "!=", "&&", "||"])
            return "((%s) %s (%s))" % (a, op, b), "i"
        if c < 0.9:
            # vírgula: o lado esquerdo é puro, só o valor do direito conta
            a, _ = self.expr(scope, depth - 1)
            b, kb = self.expr(scope, depth - 1)
            return "((void)(%s), (%s))" % (a, b), kb
        return self.leaf(scope)

    def binary(self, scope, depth):
        r = self.r
        a, ka = self.expr(scope, depth - 1)
        b, kb = self.expr(scope, depth - 1)
        op = r.choice(["+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>", "+", "-", "*", "^"])
        u = "u" in (ka, kb)
        k = "u" if u else "i"
        if op in "&|^":
            return "((%s) %s (%s))" % (a, op, b), k
        if op in ("+", "-"):
            if u:
                return "((%s) %s (%s))" % (a, op, b), "u"
            return "(((%s) >> 1) %s ((%s) >> 1))" % (a, op, b), "i"
        if op == "*":
            if u:
                return "((%s) * (%s))" % (a, b), "u"
            return "((short)(%s) * (short)(%s))" % (a, b), "i"
        if op in ("/", "%"):
            if u:
                return "((%s) %s ((%s) | 1u))" % (a, op, b), "u"
            return "((%s) %s (((%s) & 7) | 1))" % (a, op, b), "i"
        if op == "<<":
            if ka == "u":
                return "((%s) << ((%s) & 31))" % (a, b), "u"
            return "((int)((unsigned)(%s) << ((%s) & 31)))" % (a, b), "i"
        # >>: tipo do resultado é o do operando esquerdo promovido
        return "((%s) >> ((%s) & 31))" % (a, b), ka

    # ------------------------------------------------------------- comandos
    def stmt(self, scope, depth, loop_mult, in_loop, cost):
        """Gera um comando; devolve o custo estimado (passos)."""
        r = self.r
        c = r.random()
        if depth <= 0:
            c = r.random() * 0.45
        if c < 0.25:
            lv, ty = self.lvalue(scope)
            e, _ = self.expr(scope, r.randint(1, 4))
            self.emit("%s = %s;" % (lv, e))
            return loop_mult
        if c < 0.38:
            lv, ty = self.lvalue(scope)
            ops = ["+=", "-=", "*=", "^=", "|=", "&=", "/=", "%="]
            if ty == "unsigned":
                ops += ["<<=", ">>="]
            op = r.choice(ops)
            e, _ = self.expr(scope, r.randint(1, 3))
            if op in ("<<=", ">>="):
                self.emit("%s %s (%s) & 31;" % (lv, op, e))
            elif op in ("/=", "%="):
                self.emit("%s %s (unsigned)(%s) | 1u;" % (lv, op, e))
            else:
                self.emit("%s %s (unsigned)(%s);" % (lv, op, e))
            return loop_mult
        if c < 0.43:
            cands = [v for v in scope if v.writable and not v.is_array and v.ty != "int"]
            if cands:
                v = r.choice(cands)
                self.emit("%s%s;" % (v.name, r.choice(["++", "--"])) if r.random() < 0.5
                          else "%s%s;" % (r.choice(["++", "--"]), v.name))
            return loop_mult
        if c < 0.52:
            return self.call_stmt(scope, loop_mult, cost)
        if c < 0.55:
            self.struct_stmt(scope)
            return loop_mult
        if c < 0.67:
            e, _ = self.expr(scope, 3)
            self.emit("if (%s) {" % e)
            self.ind += 1
            k = self.block(scope, depth - 1, loop_mult, in_loop, cost)
            self.ind -= 1
            if r.random() < 0.5:
                self.emit("} else {")
                self.ind += 1
                k += self.block(scope, depth - 1, loop_mult, in_loop, cost + k)
                self.ind -= 1
            self.emit("}")
            return k + loop_mult
        if c < 0.8:
            n = r.randint(1, 8)
            i = self.fresh("i")
            form = r.choice(["for", "while", "do"])
            inner = scope + [Var(i, "int", writable=False)]
            if form == "for":
                self.emit("for (int %s = 0; %s < %d; %s++) {" % (i, i, n, i))
            elif form == "while":
                self.emit("{ int %s = %d; while (%s-- > 0) {" % (i, n, i))
            else:
                self.emit("{ int %s = 0; do {" % i)
            self.ind += 1
            k = self.block(inner, depth - 1, loop_mult * n, True, cost)
            if r.random() < 0.3:
                e, _ = self.expr(inner, 2)
                self.emit("if (%s) %s;" % (e, r.choice(["break", "continue"])))
            self.ind -= 1
            if form == "for":
                self.emit("}")
            elif form == "while":
                self.emit("} }")
            else:
                self.emit("} while (++%s < %d); }" % (i, n))
            return k + loop_mult * n
        if c < 0.9:
            e, _ = self.expr(scope, 3)
            self.emit("switch ((%s) & 7) {" % e)
            k = 0
            cases = sorted(r.sample(range(8), r.randint(1, 5)))
            for cv in cases:
                self.emit("case %d:" % cv)
                self.ind += 1
                k += self.block(scope, depth - 1, loop_mult, in_loop, cost + k, max_stmts=2)
                if r.random() < 0.7:
                    self.emit("break;")
                self.ind -= 1
            if r.random() < 0.6:
                self.emit("default:")
                self.ind += 1
                k += self.block(scope, depth - 1, loop_mult, in_loop, cost + k, max_stmts=2)
                self.ind -= 1
            self.emit("}")
            return k + loop_mult
        if in_loop and r.random() < 0.3:
            e, _ = self.expr(scope, 2)
            self.emit("if (%s) %s;" % (e, r.choice(["break", "continue"])))
            return loop_mult
        lv, _ = self.lvalue(scope)
        e, _ = self.expr(scope, 5)
        self.emit("%s = %s;" % (lv, e))
        return loop_mult

    def call_stmt(self, scope, loop_mult, cost):
        r = self.r
        if not self.funcs:
            return 0
        name, ret, params, fcost = r.choice(self.funcs)
        if cost + loop_mult * (fcost + 1) > MAX_FUNC_COST:
            return 0
        args = [self.arg(scope, ty, pk) for ty, _, pk in params]
        # O destino é um escalar sem índice: em `a[g & 7] = f()` a leitura
        # de g (índice) e a chamada (que pode escrever em g) não têm ordem
        # especificada; gcc e kscc podem divergir sem bug.
        lv = r.choice([v for v in scope if v.writable and not v.is_array]).name
        self.emit("%s = %s(%s);" % (lv, name, ", ".join(args)))
        return loop_mult * (fcost + 1)

    def arg(self, scope, ty, pk):
        r = self.r
        if pk == "arr":
            return r.choice([g for g in self.globals if g.is_array and g.ty == ty]).name
        if pk == "ref":
            # sempre há candidato: os campos de gs cobrem os tipos de REF_TYPES
            return "&" + r.choice([v for v in scope if v.writable and not v.is_array and v.ty == ty]).name
        return self.expr(scope, 2)[0]

    def struct_stmt(self, scope):
        """Cópia, passagem e retorno de struct por valor (mix é pura)."""
        r = self.r
        dst = r.choice(["lt", "gt[0]", "gt[1]"])
        src = r.choice(["lt", "gt[0]", "gt[1]"])
        c = r.random()
        if c < 0.4:
            e, _ = self.expr(scope, 2)
            self.emit("%s = mix(%s, %s);" % (dst, src, e))
        elif c < 0.7:
            self.emit("%s = %s;" % (dst, src))
        else:
            vals = [self.expr(scope, 1)[0] for _ in range(5)]
            self.emit("%s = (struct T){(char)(%s), (short)(%s), %s, %s, (unsigned char)(%s)};"
                      % (dst, vals[0], vals[1], vals[2], vals[3], vals[4]))

    def block(self, scope, depth, loop_mult, in_loop, cost, max_stmts=4):
        k = 0
        before = len(self.out)
        for _ in range(self.r.randint(1, max_stmts)):
            k += self.stmt(scope, depth, loop_mult, in_loop, cost + k)
        if len(self.out) == before:
            # rótulo de case/default precisa de um comando (C11 6.8.1)
            self.emit(";")
        return k

    # ------------------------------------------------------------- programa
    def gen_globals(self):
        r = self.r
        for n in range(r.randint(4, 8)):
            ty = r.choice(TYPES)
            name = "g%d" % n
            v, _ = self.const()
            self.out.append("static %s %s = %s;" % (ty, name, v))
            self.globals.append(Var(name, ty))
        for ty in ["int", "unsigned", "char", "unsigned short"]:
            name = "ga_%s" % ty.replace(" ", "_")
            vals = ", ".join(self.const()[0] for _ in range(ARR_LEN))
            self.out.append("static %s %s[%d] = {%s};" % (ty, name, ARR_LEN, vals))
            self.globals.append(Var(name, ty, is_array=True))
        self.out.append("static struct { char a; short b; int c; unsigned d; unsigned char e; } gs = {1, 2, 3, 4u, 5};")
        for f, ty in STRUCT_FIELDS:
            self.globals.append(Var("gs." + f, ty))
        self.out.append("struct T { char a; short b; int c; unsigned d; unsigned char e; };")
        self.out.append("static struct T gt[2] = {{1, 2, 3, 4u, 5}, {-1, -2, -3, 0xFFFFFFFFu, 250}};")
        self.out.append("static struct T mix(struct T x, int k) {")
        self.out.append("  x.a ^= (char)k;")
        self.out.append("  x.b = (short)(x.b + (k & 255));")
        self.out.append("  x.c = (int)((unsigned)x.c * 3u + (unsigned)k);")
        self.out.append("  x.d -= (unsigned)k;")
        self.out.append("  x.e |= (unsigned char)k;")
        self.out.append("  return x;")
        self.out.append("}")
        for i in range(2):
            for f, ty in STRUCT_FIELDS:
                self.globals.append(Var("gt[%d].%s" % (i, f), ty))
        self.out.append("")

    def gen_func(self, idx):
        r = self.r
        name = "f%d" % idx
        ret = r.choice(["int", "unsigned", "short", "unsigned char"])
        params = []
        for p in range(r.choice([0, 1, 2, 2, 3, 4, 5, 6, 8])):
            c = r.random()
            if c < 0.12:
                ty = r.choice(["int", "unsigned", "char", "unsigned short"])
                params.append((ty, "p%d" % p, "arr"))
            elif c < 0.22:
                params.append((r.choice(REF_TYPES), "p%d" % p, "ref"))
            else:
                params.append((r.choice(TYPES), "p%d" % p, "val"))
        decl = ", ".join(("%s *%s" % (t, n)) if pk != "val" else ("%s %s" % (t, n))
                         for t, n, pk in params) or "void"
        self.out.append("static %s %s(%s) {" % (ret, name, decl))
        self.ind = 1
        scope = list(self.globals)
        for t, n, pk in params:
            if pk == "ref":
                scope.append(Var("(*%s)" % n, t))
            else:
                scope.append(Var(n, t, is_array=(pk == "arr")))
        for l in range(r.randint(1, 4)):
            ty = r.choice(TYPES)
            ln = "l%d" % l
            e, _ = self.expr(scope, 2)
            self.emit("%s %s = %s;" % (ty, ln, e))
            scope.append(Var(ln, ty))
        self.emit("struct T lt = gt[%d];" % (idx & 1))
        for f, ty in STRUCT_FIELDS:
            scope.append(Var("lt." + f, ty))
        cost = self.block(scope, 3, 1, False, 0, max_stmts=6)
        e, _ = self.expr(scope, 3)
        self.emit("return %s;" % e)
        self.ind = 0
        self.out.append("}")
        self.out.append("")
        self.funcs.append((name, ret, params, cost))

    def gen_main(self):
        r = self.r
        self.out.append("int main(void) {")
        self.ind = 1
        self.emit("unsigned h = 0;")
        for name, ret, params, _ in self.funcs:
            for _ in range(r.randint(1, 3)):
                args = []
                for ty, _, pk in params:
                    if pk == "val":
                        args.append(self.const()[0])
                    else:
                        args.append(self.arg(self.globals, ty, pk))
                self.emit("h = h * 31u + (unsigned)%s(%s);" % (name, ", ".join(args)))
            self.emit('ks_put_kx("%s", h);' % name)
        for g in self.globals:
            if g.is_array:
                self.emit("for (int i = 0; i < %d; i++) h = h * 31u + (unsigned)%s[i];" % (ARR_LEN, g.name))
                self.emit('ks_put_kx("%s", h);' % g.name)
            else:
                self.emit('ks_put_kx("%s", (unsigned)%s);' % (g.name, g.name))
        self.emit('ks_put_kx("hash", h);')
        self.emit("return 0;")
        self.ind = 0
        self.out.append("}")

    def program(self):
        self.out.append("// gerado por test/fuzz/gen_expr.py, semente %d" % self.seed)
        self.out.append('#include "kstest.h"')
        self.out.append("")
        self.gen_globals()
        for i in range(self.r.randint(3, 7)):
            self.gen_func(i)
        self.gen_main()
        return "\n".join(self.out) + "\n"


class FloatGen:
    """Programa aleatório de ponto flutuante (binary32), ver o topo."""

    BUILTINS = ["sqrtf(fabsf(%s))", "fabsf(%s)", "floorf(%s)", "ceilf(%s)", "truncf(%s)",
                "rintf(%s)", "roundf(%s)"]

    def __init__(self, seed):
        self.r = random.Random(seed * 7919 + 17)
        self.seed = seed
        self.out = []
        self.fglobals = []   # nomes de float globais (escalares)
        self.iglobals = []   # nomes de int globais
        self.funcs = []      # (nome, [tipos dos parâmetros 'f'/'i'])
        self.ind = 0
        self.uid = 0

    def emit(self, line):
        self.out.append("  " * self.ind + line)

    def fconst(self):
        r = self.r
        c = r.random()
        if c < 0.25:
            v = r.choice(["0.0f", "-0.0f", "1.0f", "-1.0f", "0.5f", "2.0f", "3.0f", "0.1f",
                          "10.0f", "1e-45f", "1.17549435e-38f", "3.4028235e38f", "16777216.0f",
                          "1e30f", "-1e-30f", "0.333333343f", "7.0f", "1e10f"])
        elif c < 0.5:
            v = "%.9gf" % r.uniform(-100, 100)
        elif c < 0.65:
            v = "%.4g" % r.uniform(-10, 10)       # sem sufixo: float (-fsingle-precision-constant)
        elif c < 0.8:
            v = "%d.0f" % r.randrange(-1000, 1000) if r.random() < 0.5 else "%d.5f" % r.randrange(0, 50)
        else:
            v = "%.6ef" % (r.uniform(1, 10) * 10.0 ** r.randrange(-40, 39))
        if not v[0].isdigit() and v[0] != ".":
            v = "(" + v + ")"
        return v

    def iconst(self):
        return str(self.r.choice([0, 1, -1, 2, 3, 7, 100, -100, 255, 65536, 16777217, 2147483647]))

    def fleaf(self, fs):
        r = self.r
        c = r.random()
        if c < 0.3 or not fs:
            return self.fconst()
        if c < 0.85:
            return r.choice(fs)
        if c < 0.93:
            return "fa[(%s) & 3]" % self.iexpr([], [], 1)
        return "gs.%s" % r.choice(["x", "y"])

    def fexpr(self, fs, is_, depth):
        r = self.r
        if depth <= 0 or r.random() < 0.25:
            return self.fleaf(fs)
        c = r.random()
        if c < 0.45:
            op = r.choice(["+", "-", "*", "/", "+", "*"])
            return "(%s %s %s)" % (self.fexpr(fs, is_, depth - 1), op, self.fexpr(fs, is_, depth - 1))
        if c < 0.52:
            return "(-%s)" % self.fexpr(fs, is_, depth - 1)
        if c < 0.62:
            return "((float)(%s))" % self.iexpr(fs, is_, depth - 1)
        if c < 0.72:
            return "(%s ? %s : %s)" % (self.cond(fs, is_, depth - 1), self.fexpr(fs, is_, depth - 1),
                                       self.fexpr(fs, is_, depth - 1))
        if c < 0.82:
            return r.choice(self.BUILTINS) % self.fexpr(fs, is_, depth - 1)
        if c < 0.92 and self.funcs:
            name, params = r.choice(self.funcs)
            args = [self.fexpr(fs, is_, depth - 2) if t == "f" else self.iexpr(fs, is_, depth - 2)
                    for t in params]
            return "%s(%s)" % (name, ", ".join(args))
        return self.fleaf(fs)

    def cond(self, fs, is_, depth):
        r = self.r
        c = r.random()
        if c < 0.7:
            op = r.choice(["<", "<=", ">", ">=", "==", "!="])
            return "(%s %s %s)" % (self.fexpr(fs, is_, depth), op, self.fexpr(fs, is_, depth))
        if c < 0.8:
            return "(!%s)" % self.fexpr(fs, is_, depth)
        if c < 0.9:
            return "(%s %s %s)" % (self.cond(fs, is_, depth - 1), r.choice(["&&", "||"]),
                                   self.cond(fs, is_, depth - 1))
        return "(!!%s)" % self.fexpr(fs, is_, depth)  # float como valor de verdade

    def iexpr(self, fs, is_, depth):
        r = self.r
        c = r.random()
        if depth <= 0 or c < 0.3:
            return r.choice(is_) if is_ and r.random() < 0.6 else self.iconst()
        if c < 0.55:
            return "%s(%s)" % (r.choice(["f2i", "f2i", "f2u", "f2c"]), self.fexpr(fs, is_, depth - 1))
        if c < 0.75:
            return self.cond(fs, is_, depth - 1)
        if c < 0.9:
            return "((%s) ^ (%s))" % (self.iexpr(fs, is_, depth - 1), self.iexpr(fs, is_, depth - 1))
        return "((%s) & 0xFFFF)" % self.iexpr(fs, is_, depth - 1)

    def stmts(self, fs, is_, depth, n, mutable_f, mutable_i, in_main):
        r = self.r
        for _ in range(n):
            c = r.random()
            if depth <= 0:
                c *= 0.5
            if c < 0.3 and mutable_f:
                self.emit("%s = %s;" % (r.choice(mutable_f), self.fexpr(fs, is_, 4)))
            elif c < 0.42 and mutable_f:
                v = r.choice(mutable_f)
                op = r.choice(["+=", "-=", "*=", "/="])
                self.emit("%s %s %s;" % (v, op, self.fexpr(fs, is_, 2)))
            elif c < 0.48 and mutable_f:
                v = r.choice(mutable_f)
                pre = r.choice(["++", "--"])
                if r.random() < 0.5:
                    self.emit("%s%s;" % (pre, v))
                else:
                    t = self.fresh("t")
                    self.emit("float %s = %s%s;" % (t, v, pre))
                    self.emit("h = h * 31u + fb(%s);" % t)
            elif c < 0.55 and mutable_i:
                self.emit("%s ^= %s;" % (r.choice(mutable_i), self.iexpr(fs, is_, 3)))
            elif c < 0.62 and in_main:
                self.emit("fa[(%s) & 3] = %s;" % (self.iexpr(fs, is_, 1), self.fexpr(fs, is_, 3)))
            elif c < 0.75:
                self.emit("if %s {" % self.cond(fs, is_, 3))
                self.ind += 1
                self.stmts(fs, is_, depth - 1, r.randint(1, 3), mutable_f, mutable_i, in_main)
                self.ind -= 1
                if r.random() < 0.5:
                    self.emit("} else {")
                    self.ind += 1
                    self.stmts(fs, is_, depth - 1, r.randint(1, 3), mutable_f, mutable_i, in_main)
                    self.ind -= 1
                self.emit("}")
            elif c < 0.85:
                i = self.fresh("i")
                self.emit("for (int %s = 0; %s < %d; %s++) {" % (i, i, r.randint(1, 6), i))
                self.ind += 1
                self.stmts(fs, is_ + [i], depth - 1, r.randint(1, 3), mutable_f, mutable_i, in_main)
                self.ind -= 1
                self.emit("}")
            else:
                self.emit("h = h * 31u + fb(%s);" % self.fexpr(fs, is_, 3))

    def fresh(self, p):
        self.uid += 1
        return "%s%d" % (p, self.uid)

    def program(self):
        r = self.r
        o = self.out
        o.append("// gerado por test/fuzz/gen_expr.py %d float" % self.seed)
        o.append("#include <math.h>")
        o.append('#include "fltout.h"')
        o.append("")
        o.append("// float -> inteiro sem UB: fora do alcance e NaN dão constantes")
        o.append("static int f2i(float x) { return (x > -2147483648.0f && x < 2147483648.0f) ? (int)x : (x != x ? 7 : -9); }")
        o.append("static int f2u(float x) { return (x > -1.0f && x < 4294967296.0f) ? (int)(unsigned)x : 5; }")
        o.append("static int f2c(float x) { return (signed char)f2i(x); }")
        o.append("// bits com NaN canônico")
        o.append("static unsigned fb(float x) { return x != x ? 0x7FC00000u : fl_bits(x); }")
        o.append("")
        for k in range(r.randint(3, 6)):
            name = "g%d" % k
            o.append("static float %s = %s;" % (name, self.fconst()))
            self.fglobals.append(name)
        for k in range(r.randint(1, 3)):
            name = "n%d" % k
            o.append("static int %s = %s;" % (name, self.iconst()))
            self.iglobals.append(name)
        o.append("static float fa[4] = {%s};" % ", ".join(self.fconst() for _ in range(4)))
        o.append("static struct { float x; int k; float y; } gs = {%s, 3, %s};" % (self.fconst(), self.fconst()))
        o.append("")
        # funções puras: só leem globais e parâmetros
        for fi in range(r.randint(2, 5)):
            name = "fn%d" % fi
            params = [r.choice("ffi") for _ in range(r.randint(1, 5))]
            pnames = ["p%d" % k for k in range(len(params))]
            decl = ", ".join(("float %s" if t == "f" else "int %s") % pn for t, pn in zip(params, pnames))
            o.append("static float %s(%s) {" % (name, decl))
            self.ind = 1
            fs = self.fglobals + [pn for t, pn in zip(params, pnames) if t == "f"]
            is_ = self.iglobals + [pn for t, pn in zip(params, pnames) if t == "i"]
            locs = []
            for li in range(r.randint(1, 3)):
                ln = "l%d" % li
                self.emit("float %s = %s;" % (ln, self.fexpr(fs + locs, is_, 3)))
                locs.append(ln)
            self.emit("unsigned h = 0;")
            self.stmts(fs + locs, is_, 2, r.randint(1, 4), locs, [], False)
            self.emit("(void)h;")
            self.emit("return %s;" % self.fexpr(fs + locs, is_, 3))
            self.ind = 0
            o.append("}")
            o.append("")
            self.funcs.append((name, params))
        o.append("int main(void) {")
        self.ind = 1
        self.emit("unsigned h = 0;")
        fs = list(self.fglobals) + ["gs.x", "gs.y"]
        self.stmts(fs, list(self.iglobals), 3, r.randint(6, 14), fs, list(self.iglobals), True)
        for name, params in self.funcs:
            args = [self.fconst() if t == "f" else self.iconst() for t in params]
            self.emit("h = h * 31u + fb(%s(%s));" % (name, ", ".join(args)))
        for g in self.fglobals + ["gs.x", "gs.y", "fa[0]", "fa[1]", "fa[2]", "fa[3]"]:
            self.emit('printf("%s ");' % g)
            self.emit("fl_hex(%s);" % g)
            self.emit('printf("%%d %%.3g\\n", f2i(%s), %s != %s ? 0.0f : %s);' % (g, g, g, g))
        for g in self.iglobals:
            self.emit('printf("%s %%d\\n", %s);' % (g, g))
        self.emit('printf("hash %08x\\n", h);')
        self.emit("return 0;")
        self.ind = 0
        o.append("}")
        return "\n".join(o) + "\n"


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    if len(sys.argv) > 2 and sys.argv[2] == "float":
        sys.stdout.write(FloatGen(seed).program())
    else:
        sys.stdout.write(Gen(seed).program())


if __name__ == "__main__":
    main()
