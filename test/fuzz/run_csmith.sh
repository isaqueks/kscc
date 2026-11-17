#!/bin/bash
# test/fuzz/run_csmith.sh — fuzz diferencial com o Csmith (doc 06 §1).
#
#   test/fuzz/run_csmith.sh [N] [semente-inicial]
#
# Gera N programas (sementes consecutivas a partir de 1000, ou da dada),
# compila cada um com o gcc -m32 (referência) e com o kscc, e compara a
# saída (o checksum do Csmith). Divergências e falhas de compilação do kscc
# são copiadas para test/fuzz/failures/csmith-<semente>.c (com o motivo no
# topo). Requer csmith e libcsmith-dev (para safe_math.h); sem eles, pula.
#
# Opções do Csmith: sem float, long long, math64 (o kscc v1 não tem 64
# bits), sem volatile (sem efeito na VM e irrelevante aqui), sem structs
# packed e sem bitfields (layout de bitfield/packed é coberto por
# test/diff); unions ficam ligadas (punning little-endian igual nos dois
# alvos). Safe math fica LIGADO: com --no-safe-math os programas têm UB
# (overflow com sinal, divisão por zero) e o diferencial perde o sentido.

cd "$(dirname "$0")/../.." || exit 2
. test/fuzz/lib.sh

N=${1:-20}
SEED0=${2:-1000}
OUT=build/fuzz/csmith
RT=build/fuzz/csmith_rt
FAILDIR=test/fuzz/failures
CSMITH_OPTS="--no-float --no-longlong --no-math64 --no-packed-struct --no-volatiles
  --no-volatile-pointers --no-bitfields --no-argc --max-funcs 5 --max-block-size 4"

if ! command -v csmith > /dev/null || [ ! -f /usr/include/csmith/safe_math.h ]; then
  echo "run_csmith: csmith/libcsmith-dev nao encontrados, pulando"
  exit 0
fi

mkdir -p "$OUT" "$RT" "$FAILDIR"
# Parte inteira de safe_math.h: tudo antes do bloco de float (#ifdef __STDC__).
awk '/^#ifdef __STDC__/ { exit } { print }' /usr/include/csmith/safe_math.h > "$RT/safe_math_int.h"
echo "#endif /* SAFE_MATH_H (kscc: so a parte inteira) */" >> "$RT/safe_math_int.h"
INCS="-Itest/fuzz/csmith_rt -I$RT -Itest"

pass=0 fail=0 skip=0
for ((i = 0; i < N; i++)); do
  seed=$((SEED0 + i))
  name="csmith-$seed"
  src="$OUT/$name.c"
  csmith --seed "$seed" $CSMITH_OPTS -o "$src" > /dev/null 2>&1 || { skip=$((skip + 1)); continue; }
  diff_one "$name" "$src" "$OUT" "$INCS"
  case $verdict in
  pass) pass=$((pass + 1)) ;;
  skip-*) skip=$((skip + 1)); echo "skip $name: $detail" ;;
  fail-*)
    fail=$((fail + 1))
    echo "FAIL $name: $verdict: $detail"
    { echo "// $verdict: $detail"
      echo "// csmith --seed $seed $(echo $CSMITH_OPTS)"
      echo "// compilar com: $INCS (test/fuzz/run_csmith.sh gera $RT)"
      cat "$src"; } > "$FAILDIR/$name.c"
    ;;
  esac
done

echo
echo "csmith: $pass passaram, $fail falharam, $skip pulados (de $N; sementes $SEED0..$((SEED0 + N - 1)))"
[ $fail -eq 0 ]
