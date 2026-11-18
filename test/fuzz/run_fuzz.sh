#!/bin/bash
# test/fuzz/run_fuzz.sh — fuzz diferencial com o gerador próprio.
#
#   test/fuzz/run_fuzz.sh [N] [semente-inicial]
#
# Gera N programas inteiros com test/fuzz/gen_expr.py (sementes consecutivas
# a partir de 1, ou da dada: a mesma semente gera sempre o mesmo programa)
# e FUZZ_FLOAT_N (padrão N/2) de ponto flutuante (gen_expr.py SEMENTE
# float), compila cada um com o gcc -m32 (referência; binary32 com SSE,
# ver lib.sh) e com o kscc, roda e compara stdout e código de saída.
# Programas que divergem (ou que o kscc não compila) vão para
# test/fuzz/failures/gen-<semente>.c (gen-float-<semente>.c), com o motivo
# no topo.
#
# Com FUZZ_UBSAN=1 cada programa também roda sob -fsanitize=undefined, para
# conferir o próprio gerador (lento; não é o padrão).

cd "$(dirname "$0")/../.." || exit 2
. test/fuzz/lib.sh

N=${1:-50}
SEED0=${2:-1}
OUT=build/fuzz/gen
FAILDIR=test/fuzz/failures
PY=${PY:-python3}

mkdir -p "$OUT" "$FAILDIR"

FLOAT_N=${FUZZ_FLOAT_N:-$((N / 2))}

pass=0 fail=0 skip=0 ub=0
for ((i = 0; i < N + FLOAT_N; i++)); do
  if [ $i -lt "$N" ]; then
    seed=$((SEED0 + i))
    name="gen-$seed"
    mode=
  else
    seed=$((SEED0 + i - N))
    name="gen-float-$seed"
    mode=float
  fi
  src="$OUT/$name.c"
  $PY test/fuzz/gen_expr.py "$seed" $mode > "$src" || { echo "gerador falhou na semente $seed $mode"; exit 2; }
  if [ "${FUZZ_UBSAN:-0}" = 1 ]; then
    if $CC32 -m32 -std=c11 -O1 -w -Itest $FUZZ_FLOAT_FLAGS -fsanitize=undefined,float-cast-overflow \
         -fno-sanitize-recover=all -o "$OUT/$name.ubsan" "$src" -lm \
       && ! timeout 20 "./$OUT/$name.ubsan" > /dev/null 2> "$OUT/$name.ubsan.err"; then
      echo "UB  $name: $(grep -m1 'runtime error' "$OUT/$name.ubsan.err")"
      ub=$((ub + 1))
      continue
    fi
  fi
  diff_one "$name" "$src" "$OUT" "-Itest"
  case $verdict in
  pass) pass=$((pass + 1)) ;;
  skip-*) skip=$((skip + 1)); echo "skip $name: $detail" ;;
  fail-*)
    fail=$((fail + 1))
    echo "FAIL $name: $verdict: $detail"
    { echo "// $verdict: $detail"
      echo "// python3 test/fuzz/gen_expr.py $seed $mode; compilar com -Itest"
      cat "$src"; } > "$FAILDIR/$name.c"
    ;;
  esac
done

echo
echo "fuzz (gen_expr.py): $pass passaram, $fail falharam, $skip pulados (de $((N + FLOAT_N)): $N inteiros e $FLOAT_N float; sementes a partir de $SEED0)"
[ "$ub" -eq 0 ] || echo "ATENCAO: $ub programas com UB (bug no gerador)"
[ $fail -eq 0 ] && [ "$ub" -eq 0 ]
