#!/bin/bash
# kscc: driver.sh do chibicc adaptado ao kscc (doc 06 §2). Saem os testes
# de recursos do linker do host (.a, .so, -fPIC, -static, -shared, -L, -l,
# -Wl, de ld, -Xlinker), que não existem na KSVM; entram -###,
# -nostdlib, -Wl,--stack, arquivo inexistente, prefixo de símbolos static
# entre unidades, símbolos comuns (o ksas não tem .comm: o link do kscc
# deduplica) e referência indefinida.
kscc=$1
KSVM=${KSVM:-${KSVM2_HOME:-../ksvm2}/build/tools/ksvm}

tmp=`mktemp -d /tmp/kscc-test-XXXXXX`
trap 'rm -rf $tmp' INT TERM HUP EXIT
echo > $tmp/empty.c

check() {
    if [ $? -eq 0 ]; then
        echo "testing $1 ... passed"
    else
        echo "testing $1 ... failed"
        exit 1
    fi
}

# -o
rm -f $tmp/out
$kscc -c -o $tmp/out $tmp/empty.c
[ -f $tmp/out ]
check -o

# --help (com a atribuição ao chibicc)
$kscc --help 2>&1 | grep -q kscc
check --help
$kscc --help 2>&1 | grep -q chibicc
check '--help (atribuicao)'

# -S
echo 'int main() {}' | $kscc -S -o- -xc - | grep -q '^main:'
check -S

# Default output file
rm -f $tmp/out.kso $tmp/out.ksasm
echo 'int main() {}' > $tmp/out.c
(cd $tmp; $OLDPWD/$kscc -c out.c)
[ -f $tmp/out.kso ]
check 'default output file (.kso)'

(cd $tmp; $OLDPWD/$kscc -c -S out.c)
[ -f $tmp/out.ksasm ]
check 'default output file (.ksasm)'

# Multiple input files
rm -f $tmp/foo.kso $tmp/bar.kso
echo 'int x;' > $tmp/foo.c
echo 'int y;' > $tmp/bar.c
(cd $tmp; $OLDPWD/$kscc -c $tmp/foo.c $tmp/bar.c)
[ -f $tmp/foo.kso ] && [ -f $tmp/bar.kso ]
check 'multiple input files'

rm -f $tmp/foo.ksasm $tmp/bar.ksasm
(cd $tmp; $OLDPWD/$kscc -c -S $tmp/foo.c $tmp/bar.c)
[ -f $tmp/foo.ksasm ] && [ -f $tmp/bar.ksasm ]
check 'multiple input files'

# Run linker
rm -f $tmp/foo
echo 'int main() { return 0; }' | $kscc -o $tmp/foo -xc -xc -
$KSVM $tmp/foo
check linker

rm -f $tmp/foo
echo 'int bar(); int main() { return bar(); }' > $tmp/foo.c
echo 'int bar() { return 42; }' > $tmp/bar.c
$kscc -o $tmp/foo $tmp/foo.c $tmp/bar.c
$KSVM $tmp/foo
[ "$?" = 42 ]
check linker

# Link de .kso separados
$kscc -c -o $tmp/foo.kso $tmp/foo.c && $kscc -c -o $tmp/bar.kso $tmp/bar.c &&
  $kscc -o $tmp/foo $tmp/foo.kso $tmp/bar.kso
$KSVM $tmp/foo
[ "$?" = 42 ]
check 'link de .kso'

# static com o mesmo nome em duas unidades (prefixo _S<id>_)
echo 'static int f() { return 1; } static int v = 10; int g(); int main() { return f() + v + g(); }' > $tmp/s1.c
echo 'static int f() { return 2; } static int v = 20; int g() { return f() + v; }' > $tmp/s2.c
$kscc -o $tmp/foo $tmp/s1.c $tmp/s2.c
$KSVM $tmp/foo
[ "$?" = 33 ]
check 'static em duas unidades'

# static de mesmo nome em três unidades, inclusive static local e unidades
# compiladas antes com -c
echo 'static int n; int inc1() { static int c; return ++c + ++n; }' > $tmp/st1.c
echo 'static int n; int inc2() { static int c; c += 10; return c + (n += 100); }' > $tmp/st2.c
echo 'int inc1(); int inc2(); static int n = 1000; int main() { inc1(); inc2(); return (inc1() + inc2() + n) % 251; }' > $tmp/st3.c
$kscc -c -o $tmp/st1.kso $tmp/st1.c && $kscc -c -o $tmp/st2.kso $tmp/st2.c &&
  $kscc -o $tmp/foo $tmp/st1.kso $tmp/st2.kso $tmp/st3.c
$KSVM $tmp/foo
[ "$?" = $(( (4 + 220 + 1000) % 251 )) ]
check 'static de mesmo nome em tres unidades'

# Símbolos comuns: `int c;` em várias unidades vira um único .bss; uma
# definição com valor vence as provisórias; o tamanho é o maior.
echo 'int c; int arr[2]; int strong; void set1() { c = 1; arr[1] = 2; }' > $tmp/cm1.c
echo 'int c; int arr[8]; int strong = 40; void set2() { c += 10; arr[7] = 3; }' > $tmp/cm2.c
echo 'extern int c, arr[], strong; void set1(); void set2(); int main() { set1(); set2(); return c + arr[1] + arr[7] + strong; }' > $tmp/cm3.c
$kscc -o $tmp/foo $tmp/cm1.c $tmp/cm2.c $tmp/cm3.c
$KSVM $tmp/foo
[ "$?" = 56 ]
check 'simbolos comuns entre unidades'

$kscc -c -o $tmp/cm1.kso $tmp/cm1.c && $kscc -c -o $tmp/cm2.kso $tmp/cm2.c &&
  $kscc -o $tmp/foo $tmp/cm1.kso $tmp/cm2.kso $tmp/cm3.c
$KSVM $tmp/foo
[ "$?" = 56 ]
check 'simbolos comuns em .kso'

$kscc -S -o- $tmp/cm1.c | grep -q '^; kscc-common: c 4 4$'
check 'marcador de simbolo comum'

# -fno-common: definições provisórias viram .bss próprios, e duas
# unidades com `int c;` colidem no ksas.
! $kscc -fno-common -o $tmp/foo $tmp/cm1.c $tmp/cm2.c $tmp/cm3.c 2> $tmp/err
check '-fno-common (duplicado)'
$kscc -fno-common -S -o- $tmp/cm1.c | grep -q '^c:$'
check '-fno-common (.bss)'

# Nomes não ASCII entre unidades (escapados igualmente nas duas)
printf 'int \xcf\x80 = 3;\n' > $tmp/u1.c
printf 'extern int \xcf\x80; int main() { return \xcf\x80; }\n' > $tmp/u2.c
$kscc -o $tmp/foo $tmp/u1.c $tmp/u2.c
$KSVM $tmp/foo
[ "$?" = 3 ]
check 'identificador nao ASCII entre unidades'

# Referência indefinida
echo 'int nada(); int main() { return nada(); }' > $tmp/undef.c
$kscc -o $tmp/foo $tmp/undef.c 2>&1 | grep -q "referencia indefinida a 'nada'"
check 'referencia indefinida'

# -c aceita símbolos externos (validação com esboços)
$kscc -c -o $tmp/undef.kso $tmp/undef.c
check '-c com simbolo externo'

# a.ksx
rm -f $tmp/a.ksx
echo 'int main() {}' > $tmp/foo.c
(cd $tmp; $OLDPWD/$kscc foo.c)
[ -f $tmp/a.ksx ]
check a.ksx

# -nostdlib: sem crt0 nem runtime; _start do usuário
echo 'int _start() { return 7; }' > $tmp/ns.c
$kscc -nostdlib -o $tmp/foo $tmp/ns.c
check -nostdlib
echo 'int main() { return 0; }' > $tmp/ns2.c
! $kscc -nostdlib -o $tmp/foo $tmp/ns2.c 2> /dev/null
check '-nostdlib sem _start'

# -nostdlib com crt0 próprio em assembly, struct por valor e símbolo comum
cat > $tmp/myrt.ksasm <<'EOT'
        .text
        .global _start
_start:
        call    main
        syscall EXIT
        halt
EOT
echo 'typedef struct { int a[3]; } T; int g; T mk(int x) { T t = {{x, x, x}}; return t; } int sum(T t) { return t.a[0] + t.a[1] + t.a[2]; } int main() { g = 2; return sum(mk(4)) + g; }' > $tmp/ns3.c
$kscc -nostdlib -o $tmp/foo $tmp/myrt.ksasm $tmp/ns3.c
$KSVM $tmp/foo
[ "$?" = 14 ]
check '-nostdlib com crt0 proprio'
echo 'int ks_put_str(); int main() { return ks_put_str(); }' > $tmp/ns4.c
$kscc -nostdlib -o $tmp/foo $tmp/myrt.ksasm $tmp/ns4.c 2>&1 | grep -q "referencia indefinida a 'ks_put_str'"
check '-nostdlib sem runtime'

# -Wl,--stack=N
echo 'int f(int n) { int a[100]; a[0] = n; return n ? f(n - 1) + 1 : a[0]; } int main() { return f(50); }' > $tmp/stk.c
$kscc -o $tmp/foo $tmp/stk.c
$KSVM $tmp/foo > /dev/null 2>&1
[ "$?" = 77 ]
check '-Wl,--stack (padrao 4096: estouro)'
$kscc -Wl,--stack=65536 -o $tmp/foo $tmp/stk.c
$KSVM $tmp/foo
[ "$?" = 50 ]
check '-Wl,--stack=65536'

# -Wl,--stack com alloca/VLA: 6000 bytes não cabem na pilha padrão
echo 'int f(int n) { char v[n]; v[n - 1] = 9; return v[n - 1]; } int main() { return f(6000); }' > $tmp/stk2.c
$kscc -o $tmp/foo $tmp/stk2.c
$KSVM $tmp/foo > /dev/null 2>&1
[ "$?" = 77 ]
check '-Wl,--stack (VLA estoura a padrao)'
$kscc -Wl,--stack=16384 -o $tmp/foo $tmp/stk2.c
$KSVM $tmp/foo
[ "$?" = 9 ]
check '-Wl,--stack=16384 (VLA)'

# -###: mostra os comandos e não executa
rm -f $tmp/hash.ksx
echo 'int main() {}' > $tmp/hash.c
$kscc -### -o $tmp/hash.ksx $tmp/hash.c 2>&1 | grep -q -- '-cc1'
check -###
[ ! -f $tmp/hash.ksx ]
check '-### nao executa'

# Arquivo inexistente
! $kscc -o $tmp/foo $tmp/nao-existe.c 2> $tmp/err
check 'arquivo inexistente'
grep -q 'nao-existe.c' $tmp/err
check 'arquivo inexistente (mensagem)'

# -E
echo foo > $tmp/out
echo "#include \"$tmp/out\"" | $kscc -E -xc - | grep -q foo
check -E

echo foo > $tmp/out1
echo "#include \"$tmp/out1\"" | $kscc -E -o $tmp/out2 -xc -
cat $tmp/out2 | grep -q foo
check '-E and -o'

# -I
mkdir $tmp/dir
echo foo > $tmp/dir/i-option-test
echo "#include \"i-option-test\"" | $kscc -I$tmp/dir -E -xc - | grep -q foo
check -I
echo "#include \"i-option-test\"" | $kscc -I $tmp/dir -E -xc - | grep -q foo
check '-I (separado)'

# -D
echo foo | $kscc -Dfoo -E -xc - | grep -q 1
check -D

# -D
echo foo | $kscc -Dfoo=bar -E -xc - | grep -q bar
check -D

echo foo | $kscc -D foo=bar -E -xc - | grep -q bar
check '-D (separado)'

# -U
echo foo | $kscc -Dfoo=bar -Ufoo -E -xc - | grep -q foo
check -U

# Macros predefinidas (doc 04 §4)
echo '__ksvm__ __ILP32__ __SIZEOF_LONG__ __SIZEOF_POINTER__ __kscc__' | $kscc -E -xc - | grep -q '^1 1 4 4 1$'
check 'macros predefinidas'
echo '__x86_64__ __linux__ __LP64__' | $kscc -E -xc - | grep -q '^__x86_64__ __linux__ __LP64__$'
check 'sem macros de x86-64'

# ignored options
$kscc -c -O -Wall -g -std=c11 -ffreestanding -fno-builtin \
         -fno-omit-frame-pointer -fno-stack-protector -fno-strict-aliasing \
         -w -o /dev/null $tmp/empty.c
check 'ignored options'

# BOM marker
printf '\xef\xbb\xbfxyz\n' | $kscc -E -o- -xc - | grep -q '^xyz'
check 'BOM marker'

# Inline functions
echo 'inline void foo() {}' > $tmp/inline1.c
echo 'inline void foo() {}' > $tmp/inline2.c
echo 'int main() { return 0; }' > $tmp/inline3.c
$kscc -o /dev/null $tmp/inline1.c $tmp/inline2.c $tmp/inline3.c
check inline

echo 'extern inline void foo() {}' > $tmp/inline1.c
echo 'int foo(); int main() { foo(); }' > $tmp/inline2.c
$kscc -o /dev/null $tmp/inline1.c $tmp/inline2.c
check inline

echo 'static inline void f1() {}' | $kscc -o- -S -xc - | grep -v -q f1:
check inline

echo 'static inline void f1() {} void foo() { f1(); }' | $kscc -o- -S -xc - | grep -q f1:
check inline

echo 'static inline void f1() {} static inline void f2() { f1(); } void foo() { f1(); }' | $kscc -o- -S -xc - | grep -q f1:
check inline

echo 'static inline void f1() {} static inline void f2() { f1(); } void foo() { f1(); }' | $kscc -o- -S -xc - | grep -v -q f2:
check inline

echo 'static inline void f1() {} static inline void f2() { f1(); } void foo() { f2(); }' | $kscc -o- -S -xc - | grep -q f1:
check inline

echo 'static inline void f1() {} static inline void f2() { f1(); } void foo() { f2(); }' | $kscc -o- -S -xc - | grep -q f2:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() {}' | $kscc -o- -S -xc - | grep -v -q f1:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() {}' | $kscc -o- -S -xc - | grep -v -q f2:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() { f1(); }' | $kscc -o- -S -xc - | grep -q f1:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() { f1(); }' | $kscc -o- -S -xc - | grep -q f2:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() { f2(); }' | $kscc -o- -S -xc - | grep -q f1:
check inline

echo 'static inline void f2(); static inline void f1() { f2(); } static inline void f2() { f1(); } void foo() { f2(); }' | $kscc -o- -S -xc - | grep -q f2:
check inline

# -idirafter
mkdir -p $tmp/dir1 $tmp/dir2
echo foo > $tmp/dir1/idirafter
echo bar > $tmp/dir2/idirafter
echo "#include \"idirafter\"" | $kscc -I$tmp/dir1 -I$tmp/dir2 -E -xc - | grep -q foo
check -idirafter
echo "#include \"idirafter\"" | $kscc -idirafter $tmp/dir1 -I$tmp/dir2 -E -xc - | grep -q bar
check -idirafter

# -include
echo foo > $tmp/out.h
echo bar | $kscc -include $tmp/out.h -E -o- -xc - | grep -q -z 'foo.*bar'
check -include
# kscc: stddef.h do kscc no lugar do stdio.h do sistema (sem libc até a fase 4)
echo NULL | $kscc -Iinclude -include stddef.h -E -o- -xc - | grep -q 0
check -include

# -x
echo 'int x;' | $kscc -c -xc -o $tmp/foo.kso -
check -xc
echo 'x:' | $kscc -c -x assembler -o $tmp/foo.kso -
check '-x assembler'

echo 'int x;' > $tmp/foo.c
$kscc -c -x assembler -x none -o $tmp/foo.kso $tmp/foo.c
check '-x none'

# -E
echo foo | $kscc -E - | grep -q foo
check -E

$kscc -hashmap-test
check 'hashmap'

# -M
echo '#include "out2.h"' > $tmp/out.c
echo '#include "out3.h"' >> $tmp/out.c
touch $tmp/out2.h $tmp/out3.h
$kscc -M -I$tmp $tmp/out.c | grep -q -z '^out.kso: .*/out\.c .*/out2\.h .*/out3\.h'
check -M

# -MF
$kscc -MF $tmp/mf -M -I$tmp $tmp/out.c
grep -q -z '^out.kso: .*/out\.c .*/out2\.h .*/out3\.h' $tmp/mf
check -MF

# -MP
$kscc -MF $tmp/mp -MP -M -I$tmp $tmp/out.c
grep -q '^.*/out2.h:' $tmp/mp
check -MP
grep -q '^.*/out3.h:' $tmp/mp
check -MP

# -MT
$kscc -MT foo -M -I$tmp $tmp/out.c | grep -q '^foo:'
check -MT
$kscc -MT foo -MT bar -M -I$tmp $tmp/out.c | grep -q '^foo bar:'
check -MT

# -MD
echo '#include "out2.h"' > $tmp/md2.c
echo '#include "out3.h"' > $tmp/md3.c
(cd $tmp; $OLDPWD/$kscc -c -MD -I. md2.c md3.c)
grep -q -z '^md2.kso:.* md2\.c .* ./out2\.h' $tmp/md2.d
check -MD
grep -q -z '^md3.kso:.* md3\.c .* ./out3\.h' $tmp/md3.d
check -MD

$kscc -c -MD -MF $tmp/md-mf.d -I. $tmp/md2.c
grep -q -z '^md2.kso:.*md2\.c .*/out2\.h' $tmp/md-mf.d
check -MD

# #include_next
mkdir -p $tmp/next1 $tmp/next2 $tmp/next3
echo '#include "file1.h"' > $tmp/file.c
echo '#include_next "file1.h"' > $tmp/next1/file1.h
echo '#include_next "file2.h"' > $tmp/next2/file1.h
echo 'foo' > $tmp/next3/file2.h
$kscc -I$tmp/next1 -I$tmp/next2 -I$tmp/next3 -E $tmp/file.c | grep -q foo
check '#include_next'

# Opções do linker do host não existem
! $kscc -static -o $tmp/foo $tmp/foo.c 2> /dev/null
check '-static rejeitado'

echo OK
