#!/bin/bash
# tools/run_all_tests.sh — todas as camadas de teste do kscc (doc 06 §1):
#
#   1. test/run.sh            unitários + suite do chibicc adaptada
#   2. test/chibicc/driver.sh opções do driver
#   3. test/diff/run.sh       diferencial kscc x gcc -m32 (+ UBSan nos testes)
#   4. test/fuzz/run_fuzz.sh  fuzz com o gerador próprio (FUZZ_N, padrão 50)
#   5. test/fuzz/run_csmith.sh  Csmith (CSMITH_N, padrão 0 = pula; precisa
#                             de csmith + libcsmith-dev)
#   6. test/bench/run.sh      benchmarks (só confere a saída; o número de
#                             instruções é informativo)
#   7. test/embedded/run.sh   hello/blink/selftest no simavr e no QEMU
#
#   tools/run_all_tests.sh [caminho/do/kscc]
#
# Mostra um resumo e sai com código != 0 se alguma camada falhar. O kscc é
# construído antes, com a libks (make). Variáveis: KSVM2_HOME (padrão ../ksvm2),
# FUZZ_N, CSMITH_N.

cd "$(dirname "$0")/.." || exit 2

export KSVM2_HOME=${KSVM2_HOME:-../ksvm2}
export KSAS=${KSAS:-$(cd "$KSVM2_HOME" && pwd)/build/ksas}
export KSVM=${KSVM:-$(cd "$KSVM2_HOME" && pwd)/build/tools/ksvm}
export KSCC=${1:-build/kscc}
FUZZ_N=${FUZZ_N:-50}
CSMITH_N=${CSMITH_N:-0}
LOGDIR=build/test-all
mkdir -p "$LOGDIR"

if [ "$KSCC" = build/kscc ]; then
  # a libks também: ela é compilada pelo kscc e fica velha se ele mudou
  make -s --no-print-directory build/kscc libks || exit 2
fi

names=()
results=()
fail=0

step() {
  local name=$1
  shift
  local log="$LOGDIR/$(echo "$name" | tr ' /' '__').log"
  printf '%-22s ' "$name"
  "$@" > "$log" 2>&1
  local rc=$?
  local summary
  summary=$(grep -E 'passaram|OK$|embarcado:|FALHAS' "$log" | tail -2 | tr '\n' ' ')
  if [ $rc -eq 0 ]; then
    echo "ok    $summary"
    results+=("ok   $name")
  else
    printf '\033[31mFALHOU\033[0m (rc=%d, log: %s)\n' $rc "$log"
    tail -15 "$log" | sed 's/^/    /'
    results+=("FAIL $name ($log)")
    fail=1
  fi
}

step "unit+chibicc" test/run.sh "$KSCC"
step "driver" test/chibicc/driver.sh "$KSCC"
step "diff" test/diff/run.sh
step "fuzz gen_expr ($FUZZ_N)" test/fuzz/run_fuzz.sh "$FUZZ_N"
if [ "$CSMITH_N" -gt 0 ]; then
  step "fuzz csmith ($CSMITH_N)" test/fuzz/run_csmith.sh "$CSMITH_N"
fi
step "bench" test/bench/run.sh
step "embedded" test/embedded/run.sh

echo
echo "== resumo =="
for r in "${results[@]}"; do echo "  $r"; done
if [ $fail -ne 0 ]; then
  echo "run_all_tests: FALHOU"
  exit 1
fi
echo "run_all_tests: OK"
