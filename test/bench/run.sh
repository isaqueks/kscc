#!/bin/bash
# test/bench/run.sh — instruções executadas pela KSVM em cada benchmark
# (doc 06 §6, métrica que a fase 7 vai otimizar).
#
#   test/bench/run.sh            mede e compara com test/bench/baseline.txt
#   test/bench/run.sh --update   mede e regrava a baseline
#
# O número vem do "insns=N" que `ksvm --regs` imprime ao terminar (não há
# syscall para ler INSN_COUNT de dentro do programa). A saída de cada
# programa é conferida contra o gcc -m32 (um benchmark que calcula errado
# não vale). Também mostra o tamanho do .ksx.
# Falha se a saída divergir; a variação de instruções só é informativa.

cd "$(dirname "$0")/../.." || exit 2

KSCC=${KSCC:-build/kscc}
KSVM=${KSVM:-${KSVM2_HOME:-../ksvm2}/build/tools/ksvm}
CC32=${CC32:-gcc}
OUT=build/bench
BASE=test/bench/baseline.txt
BENCHES="fib sieve qsort_bench strlen_loop matmul"
mkdir -p "$OUT"

update=0
[ "${1:-}" = --update ] && update=1

fail=0
results=()
printf '%-14s %14s %14s %9s %9s\n' benchmark instrucoes baseline delta ksx
for b in $BENCHES; do
  src=test/bench/$b.c
  if ! "$KSCC" -Itest -o "$OUT/$b.ksx" "$src" 2> "$OUT/$b.cerr"; then
    echo "FAIL $b: kscc nao compilou: $(head -1 "$OUT/$b.cerr")"; fail=1; continue
  fi
  "$KSVM" --regs --max-insns 2000000000 "$OUT/$b.ksx" > "$OUT/$b.out" 2> "$OUT/$b.err"
  rc=$?
  insns=$(sed -n 's/.* insns=\([0-9]*\).*/\1/p' "$OUT/$b.err" | tail -1)
  $CC32 -m32 -std=c11 -O1 -w -Itest -o "$OUT/$b.gcc" "$src" && "./$OUT/$b.gcc" > "$OUT/$b.gcc.out"
  if [ $rc -ne 0 ] || [ -z "$insns" ] || ! cmp -s "$OUT/$b.out" "$OUT/$b.gcc.out"; then
    echo "FAIL $b: rc=$rc, saida difere do gcc ou sem contagem: $(head -2 "$OUT/$b.err")"
    fail=1
    continue
  fi
  size=$(stat -c %s "$OUT/$b.ksx")
  base=$(awk -v b="$b" '$1 == b { print $2 }' "$BASE" 2>/dev/null)
  if [ -n "$base" ] && [ "$base" -gt 0 ]; then
    delta=$(awk -v n="$insns" -v o="$base" 'BEGIN { printf "%+.1f%%", (n - o) * 100 / o }')
  else
    base=-; delta=-
  fi
  printf '%-14s %14s %14s %9s %9s\n' "$b" "$insns" "$base" "$delta" "$size"
  results+=("$b $insns $size")
done

if [ $update = 1 ] && [ $fail = 0 ]; then
  {
    echo "# test/bench/baseline.txt — instrucoes executadas (ksvm --regs) e tamanho do .ksx"
    echo "# gerado por test/bench/run.sh --update (kscc v1, codegen ingenuo da fase 4)"
    echo "# benchmark instrucoes bytes_ksx"
    for r in "${results[@]}"; do echo "$r"; done
  } > "$BASE"
  echo "baseline gravada em $BASE"
fi
[ $fail = 0 ] && echo OK
exit $fail
