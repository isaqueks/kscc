#!/bin/bash
# test/run.sh — harness de testes do kscc (doc 06).
#
#   test/run.sh [caminho/do/kscc] [filtro]
#
# 1. test/unit/*.c: cada arquivo é compilado com o kscc (e a libks),
#    executado no ksvm (--max-insns 50000000) e comparado com os
#    cabeçalhos:
#      // expect-exit: N                código de saída (padrão 0)
#      // expect-out: texto             uma linha de stdout (repetível; a
#                                       saída esperada é a concatenação)
#      // expect-noeol                  a saída termina sem '\n' (tira o
#                                       '\n' da última linha esperada)
#      // expect-error: texto           a compilação deve FALHAR e a
#                                       mensagem conter `texto`
#      // cflags: opções               opções extras para o kscc
#      // stdin: texto                  uma linha da entrada padrão do
#                                       programa (repetível; sem nenhuma,
#                                       a entrada é vazia)
# 2. examples/*.c: os programas de exemplo, com os mesmos cabeçalhos.
# 3. test/chibicc/*.c (menos os listados em test/chibicc/SKIP): compilado
#    com common.c; passa se sai com 0 e a última linha é "OK".
#
# Variáveis: KSAS, KSVM (caminhos), KSCC_TEST_TRACE=1 (reexecuta o teste que
# falhou com --trace e guarda o trace em build/test/<nome>.trace).

KSCC=${1:-build/kscc}
FILTER=${2:-}
KSVM=${KSVM:-${KSVM2_HOME:-../ksvm2}/build/tools/ksvm}
MAX_INSNS=50000000
OUT=build/test

mkdir -p "$OUT"

pass=0
fail=0
skip=0
failed=()

red() { printf '\033[31m%s\033[0m\n' "$*"; }

# Reexecuta com --trace se pedido.
trace() {
  local name=$1 ksx=$2
  if [ "${KSCC_TEST_TRACE:-0}" = 1 ]; then
    "$KSVM" --max-insns $MAX_INSNS --trace "$ksx" > /dev/null 2> "$OUT/$name.trace"
    echo "    trace: $OUT/$name.trace"
  fi
}

report_fail() {
  local name=$1
  shift
  red "FAIL $name"
  for line in "$@"; do echo "    $line"; done
  fail=$((fail + 1))
  failed+=("$name")
}

# ---------------------------------------------------------------------------
# Unitários
# ---------------------------------------------------------------------------
# run_unit arquivo.c [grupo] — grupo "unit" (padrão) ou "examples".
run_unit() {
  local src=$1
  local group=${2:-unit}
  local name="$group/$(basename "$src" .c)"
  local base="$OUT/$group-$(basename "$src" .c)"
  local ksx="$base.ksx"

  local exp_exit exp_err cflags
  cflags=$(sed -n 's|^// cflags: *||p' "$src" | head -1)
  exp_exit=$(sed -n 's|^// expect-exit: *||p' "$src" | head -1)
  exp_exit=${exp_exit:-0}
  exp_err=$(sed -n 's|^// expect-error: *||p' "$src" | head -1)
  sed -n 's|^// expect-out: \{0,1\}||p' "$src" > "$base.expected"
  sed -n 's|^// stdin: \{0,1\}||p' "$src" > "$base.stdin"
  if grep -q '^// expect-noeol' "$src" && [ -s "$base.expected" ]; then
    truncate -s -1 "$base.expected"
  fi

  "$KSCC" -Itest/unit $cflags -o "$ksx" "$src" 2> "$base.cerr"
  local crc=$?

  if [ -n "$exp_err" ]; then
    if [ $crc -eq 0 ]; then
      report_fail "$name" "compilou, mas devia falhar com: $exp_err"
    elif ! grep -qF -- "$exp_err" "$base.cerr"; then
      report_fail "$name" "erro de compilação diferente do esperado ($exp_err):" \
        "$(head -5 "$base.cerr")"
    else
      pass=$((pass + 1))
    fi
    return
  fi

  if [ $crc -ne 0 ]; then
    report_fail "$name" "falha de compilação:" "$(head -20 "$base.cerr")"
    return
  fi

  "$KSVM" --max-insns $MAX_INSNS "$ksx" < "$base.stdin" > "$base.out" 2> "$base.err"
  local rc=$?

  local msgs=()
  if [ "$rc" != "$exp_exit" ]; then
    msgs+=("código de saída $rc, esperado $exp_exit")
    [ -s "$base.err" ] && msgs+=("ksvm: $(head -3 "$base.err")")
  fi
  if ! cmp -s "$base.expected" "$base.out"; then
    msgs+=("stdout difere (- esperado, + obtido):")
    while IFS= read -r l; do msgs+=("$l"); done < <(diff -u "$base.expected" "$base.out" | tail -n +3 | head -40)
  fi

  if [ ${#msgs[@]} -eq 0 ]; then
    pass=$((pass + 1))
  else
    report_fail "$name" "${msgs[@]}"
    trace "$group-$(basename "$src" .c)" "$ksx"
  fi
}

# ---------------------------------------------------------------------------
# Suite do chibicc
# ---------------------------------------------------------------------------
SKIPFILE=test/chibicc/SKIP

is_skipped() {
  grep -q "^$1[[:space:]]" "$SKIPFILE" 2>/dev/null
}

run_chibicc() {
  local src=$1
  local file=$(basename "$src")
  local name="chibicc/$(basename "$src" .c)"
  local base="$OUT/chibicc-$(basename "$src" .c)"
  local ksx="$base.ksx"

  if is_skipped "$file"; then
    skip=$((skip + 1))
    return
  fi

  # kscc: os testes do chibicc alocam muitas locais em main (cada ({ ... })
  # tem as suas); a pilha padrão de 4096 bytes não basta.
  "$KSCC" -Iinclude -Itest/chibicc -Wl,--stack=131072 -o "$ksx" "$src" test/chibicc/common.c 2> "$base.cerr"
  if [ $? -ne 0 ]; then
    report_fail "$name" "falha de compilação:" "$(head -20 "$base.cerr")"
    return
  fi

  "$KSVM" --max-insns $MAX_INSNS "$ksx" > "$base.out" 2> "$base.err"
  local rc=$?
  local last
  last=$(tail -n 1 "$base.out")

  if [ $rc -eq 0 ] && [ "$last" = OK ]; then
    pass=$((pass + 1))
  else
    report_fail "$name" "código de saída $rc, última linha: '$last'" \
      "$(grep -v ' => ' "$base.out" | tail -3)" \
      "$(grep 'expected but got' "$base.out" | head -3)" \
      "$(head -3 "$base.err")"
    trace "chibicc-$(basename "$src" .c)" "$ksx"
  fi
}

unit_files=$(ls test/unit/*.c 2>/dev/null | grep -- "$FILTER")
example_files=$(ls examples/*.c 2>/dev/null | grep -- "$FILTER")
chibicc_files=$(ls test/chibicc/*.c 2>/dev/null | grep -v '/common\.c$' | grep -- "$FILTER")

for f in $unit_files; do run_unit "$f"; done
unit_pass=$pass
unit_fail=$fail

for f in $example_files; do run_unit "$f" examples; done
example_pass=$((pass - unit_pass))
example_fail=$((fail - unit_fail))

for f in $chibicc_files; do run_chibicc "$f"; done
chibicc_pass=$((pass - unit_pass - example_pass))
chibicc_fail=$((fail - unit_fail - example_fail))

echo
echo "unitários:         $unit_pass passaram, $unit_fail falharam"
echo "exemplos:          $example_pass passaram, $example_fail falharam"
echo "suite do chibicc:  $chibicc_pass passaram, $chibicc_fail falharam, $skip pulados (ver $SKIPFILE)"

# Nenhum teste encontrado (filtro sem casamento, diretório errado) não é
# sucesso.
if [ $((pass + fail + skip)) -eq 0 ]; then
  red "nenhum teste rodou"
  exit 1
fi
if [ $fail -ne 0 ]; then
  red "FALHAS: ${failed[*]}"
  exit 1
fi
echo OK
