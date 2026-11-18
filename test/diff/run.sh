#!/bin/bash
# test/diff/run.sh — testes diferenciais kscc x gcc -m32 (doc 06 §1).
#
#   test/diff/run.sh [filtro]
#
# Para cada test/diff/*.c e test/diff/regressions/*.c:
#   1. kscc -Itest → .ksx → ksvm --max-insns 200000000
#   2. gcc -m32 -std=c11 -O1 -fwrapv -fno-strict-aliasing -Wall → nativo, com
#      ponto flutuante em binary32 igual ao da KSVM: -msse -mfpmath=sse
#      (sem precisão estendida do x87), -fexcess-precision=standard e
#      -fsingle-precision-constant (literais sem sufixo são float, como no
#      kscc); -lm. Os programas float só usam float (nunca double), e
#      imprimem NaN como "nan" (test/fltout.h): o x86 produz 0xFFC00000 e a
#      KSVM o canônico 0x7FC00000.
#   3. compara stdout e código de saída (diff -u legível em caso de falha)
# Com DIFF_UBSAN=1 (padrão) o fonte também é compilado com
# -fsanitize=undefined (sem -fwrapv) e executado: se o UBSan reclamar, o
# programa de teste é inválido e o caso falha como "UB no teste".
#
# Cabeçalho opcional no .c:  // kscc-flags: opções extras para o kscc
#
# Variáveis: KSCC (padrão build/kscc), KSVM, KSVM2_HOME, CC32 (padrão gcc),
# DIFF_UBSAN (1/0), DIFF_DIR (diretório dos fontes; padrão test/diff).
# Saídas em build/test/diff/.

cd "$(dirname "$0")/../.." || exit 2

FILTER=${1:-}
KSCC=${KSCC:-build/kscc}
KSVM=${KSVM:-${KSVM2_HOME:-../ksvm2}/build/tools/ksvm}
CC32=${CC32:-gcc}
DIFF_UBSAN=${DIFF_UBSAN:-1}
DIFF_DIR=${DIFF_DIR:-test/diff}
MAX_INSNS=200000000
OUT=build/test/diff
FLOAT_FLAGS="-msse -mfpmath=sse -fexcess-precision=standard -fsingle-precision-constant"
GCC_FLAGS="-m32 -std=c11 -O1 -fwrapv -fno-strict-aliasing -Wall -Itest $FLOAT_FLAGS"
UBSAN_FLAGS="-m32 -std=c11 -O1 -g -fno-strict-aliasing -Itest $FLOAT_FLAGS -fsanitize=undefined,float-cast-overflow -fno-sanitize-recover=all"

mkdir -p "$OUT"

red() { printf '\033[31m%s\033[0m\n' "$*"; }

pass=0
fail=0
failed=()

report_fail() {
  local name=$1
  shift
  red "FAIL $name"
  for line in "$@"; do echo "    $line"; done
  fail=$((fail + 1))
  failed+=("$name")
}

run_one() {
  local src=$1
  local name=${src#test/}
  name=${name%.c}
  local base="$OUT/$(echo "${src#$DIFF_DIR/}" | sed 's|/|-|g; s|\.c$||')"
  local kflags
  kflags=$(sed -n 's|^// kscc-flags: *||p' "$src" | head -1)

  # --- gcc ---
  if ! $CC32 $GCC_FLAGS -o "$base.gcc" "$src" -lm 2> "$base.gcc.cerr"; then
    report_fail "$name" "gcc nao compilou:" "$(head -10 "$base.gcc.cerr")"
    return
  fi
  if [ -s "$base.gcc.cerr" ]; then
    report_fail "$name" "avisos do gcc (os testes devem compilar limpos com -Wall):" \
      "$(head -10 "$base.gcc.cerr")"
    return
  fi
  timeout 20 "./$base.gcc" > "$base.gcc.out" 2> /dev/null
  local grc=$?

  if [ "$DIFF_UBSAN" = 1 ]; then
    if ! $CC32 $UBSAN_FLAGS -o "$base.ubsan" "$src" -lm 2> "$base.ubsan.cerr"; then
      report_fail "$name" "gcc+UBSan nao compilou:" "$(head -10 "$base.ubsan.cerr")"
      return
    fi
    timeout 20 "./$base.ubsan" > /dev/null 2> "$base.ubsan.err"
    if grep -q 'runtime error' "$base.ubsan.err"; then
      report_fail "$name" "UB no teste (UBSan):" "$(grep 'runtime error' "$base.ubsan.err" | head -5)"
      return
    fi
  fi

  # --- kscc ---
  if ! "$KSCC" -Itest -Wl,--stack=65536 $kflags -o "$base.ksx" "$src" 2> "$base.cerr"; then
    report_fail "$name" "kscc nao compilou:" "$(head -20 "$base.cerr")"
    return
  fi
  "$KSVM" --max-insns $MAX_INSNS "$base.ksx" > "$base.out" 2> "$base.err"
  local krc=$?

  local msgs=()
  if [ "$krc" != "$grc" ]; then
    msgs+=("codigo de saida: kscc $krc, gcc $grc")
    [ -s "$base.err" ] && msgs+=("ksvm: $(head -3 "$base.err")")
  fi
  if ! cmp -s "$base.gcc.out" "$base.out"; then
    msgs+=("stdout difere (- gcc, + kscc):")
    while IFS= read -r l; do msgs+=("$l"); done < <(diff -u "$base.gcc.out" "$base.out" | tail -n +3 | head -40)
  fi
  if [ ${#msgs[@]} -eq 0 ]; then
    pass=$((pass + 1))
    echo "ok   $name ($(wc -l < "$base.out") linhas)"
  else
    report_fail "$name" "${msgs[@]}"
  fi
}

for src in $(ls "$DIFF_DIR"/*.c "$DIFF_DIR"/regressions/*.c 2>/dev/null | grep -- "$FILTER"); do
  run_one "$src"
done

echo
echo "diferencial: $pass passaram, $fail falharam"
# Nenhum teste encontrado (filtro sem casamento, diretório errado) não é
# sucesso.
if [ $((pass + fail)) -eq 0 ]; then
  red "nenhum teste rodou"
  exit 1
fi
if [ $fail -ne 0 ]; then
  red "FALHAS: ${failed[*]}"
  exit 1
fi
echo OK
