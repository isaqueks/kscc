# test/fuzz/lib.sh — parte comum de run_csmith.sh e run_fuzz.sh (source).
#
# diff_one <nome> <fonte.c> <dir de saída> <flags extras de include>
#   compila com gcc -m32 (referência) e com o kscc, roda os dois e compara
#   stdout + código de saída. Define $verdict:
#     pass          saídas iguais
#     skip-gcc      a referência não compilou ou não terminou em $GCC_TIMEOUT s
#     skip-insns    o ksvm esgotou --max-insns (programa longo demais)
#     fail-compile  o kscc não compilou um programa que o gcc compila
#     fail-run      saídas ou códigos de saída diferentes
# e $detail com o motivo (uma linha).
#
# O gcc usa ponto flutuante binary32 como o da KSVM (doc 06 §1): SSE em vez
# do x87, sem precisão estendida nem contração em FMA, literais sem sufixo
# em float; -lm para <math.h>. Nada disso muda os programas inteiros.

KSCC=${KSCC:-build/kscc}
KSVM=${KSVM:-${KSVM2_HOME:-../ksvm2}/build/tools/ksvm}
CC32=${CC32:-gcc}
FUZZ_MAX_INSNS=${FUZZ_MAX_INSNS:-500000000}
GCC_TIMEOUT=${GCC_TIMEOUT:-10}
FUZZ_FLOAT_FLAGS="-msse -mfpmath=sse -fexcess-precision=standard -fsingle-precision-constant -ffp-contract=off"

diff_one() {
  local name=$1 src=$2 out=$3 incs=$4
  local base="$out/$name"
  verdict=pass
  detail=

  if ! $CC32 -m32 -std=c11 -O1 -w -fwrapv -fno-strict-aliasing $FUZZ_FLOAT_FLAGS $incs \
       -o "$base.gcc" "$src" -lm 2> "$base.gcc.cerr"; then
    verdict=skip-gcc; detail="gcc nao compilou: $(head -1 "$base.gcc.cerr")"; return
  fi
  timeout "$GCC_TIMEOUT" "./$base.gcc" > "$base.gcc.out" 2> /dev/null
  local grc=$?
  if [ $grc -eq 124 ]; then
    verdict=skip-gcc; detail="gcc: timeout de ${GCC_TIMEOUT}s"; return
  fi

  if ! "$KSCC" $incs -Wl,--stack=262144 -o "$base.ksx" "$src" > "$base.cerr" 2>&1; then
    verdict=fail-compile
    detail="kscc nao compilou: $(grep -m1 . "$base.cerr")"
    [ -s "$base.cerr" ] || detail="kscc nao compilou (sem mensagem; cc1 morto por sinal?)"
    return
  fi
  "$KSVM" --max-insns "$FUZZ_MAX_INSNS" "$base.ksx" > "$base.out" 2> "$base.err"
  local krc=$?
  if [ $krc -eq 124 ] && grep -q "instrucoes atingido" "$base.err"; then
    verdict=skip-insns; detail="ksvm: $(head -1 "$base.err")"; return
  fi
  if [ "$krc" != "$grc" ] || ! cmp -s "$base.gcc.out" "$base.out"; then
    verdict=fail-run
    detail="saida: kscc rc=$krc '$(head -c 60 "$base.out" | tr '\n' ' ')' gcc rc=$grc '$(head -c 60 "$base.gcc.out" | tr '\n' ' ')' $(head -1 "$base.err")"
  fi
}
