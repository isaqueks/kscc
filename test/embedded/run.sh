#!/bin/bash
# test/embedded/run.sh — programas C embarcados (doc 06 §1, camada
# "Embarcado"): hello, blink e selftest compilados pelo kscc e rodados como
# firmware no ATmega328P (simavr) e no Cortex-M3 do mps2-an385 (QEMU), via
# os alvos avr-custom/arm-custom da ksvm2; a saída da UART é conferida com
# ports/common/expect_run.sh da ksvm2.
#
#   test/embedded/run.sh
#
# Antes dos simuladores, cada programa roda no ksvm do desktop com
# --ram 1024 (a RAM do guest no AVR), e o selftest também é comparado com o
# gcc -m32. Plataforma sem toolchain/simulador é pulada com aviso.
#
# Os wrappers de GPIO/tempo (ks_sys_pin_*, ks_sys_sleep_ms, ks_sys_millis,
# ks_sys_caps) vêm de test/embedded/gpio.ksasm enquanto o runtime do kscc
# não os tiver (o kscc aceita .ksasm na linha de comando e concatena).
#
# Variáveis: KSCC, KSVM2_HOME (padrão ../ksvm2), EMB_STACK (pilha do guest,
# padrão 768: no AVR .data + .bss + pilha cabem em 1 KB).

cd "$(dirname "$0")/../.." || exit 2
ROOT=$(pwd)

KSCC=${KSCC:-build/kscc}
KSVM2_HOME=${KSVM2_HOME:-../ksvm2}
KSVM2_ABS=$(cd "$KSVM2_HOME" && pwd)
KSVM=${KSVM:-$KSVM2_ABS/build/tools/ksvm}
CC32=${CC32:-gcc}
EMB_STACK=${EMB_STACK:-768}
OUT=build/embedded
EXPECT="sh $KSVM2_ABS/ports/common/expect_run.sh"
AVR_FLASH=32768
mkdir -p "$OUT"

red() { printf '\033[31m%s\033[0m\n' "$*"; }

PROGS="hello blink selftest"
expected() {
  case $1 in
  hello) echo "KSVM C on $2" ;;
  blink) echo "blink ok" ;;
  selftest) echo "selftest ok" ;;
  esac
}

# gpio.ksasm só se o runtime ainda não define os wrappers.
EXTRA=test/embedded/gpio.ksasm
if grep -qs '^ks_sys_pin_mode:' lib/libks.kso lib/minirt.ksasm; then
  EXTRA=
fi

fail=0
failed=()
report=()

note_fail() {
  red "FAIL $1"
  fail=1
  failed+=("$1")
}

build() { # prog plataforma -> $OUT/prog-plat.ksx
  local p=$1 plat=$2
  "$KSCC" -Itest -DKS_PLATFORM="\"$plat\"" -Wl,--stack=$EMB_STACK \
    -o "$OUT/$p-$plat.ksx" "test/embedded/$p.c" $EXTRA 2> "$OUT/$p-$plat.cerr"
}

# --- 1. desktop: ksvm com a RAM do AVR -------------------------------------
echo "== desktop (ksvm --ram 1024) =="
for p in $PROGS; do
  if ! build "$p" desktop; then
    note_fail "$p/desktop: kscc nao compilou: $(head -3 "$OUT/$p-desktop.cerr")"
    continue
  fi
  "$KSVM" --ram 1024 --max-insns 50000000 "$OUT/$p-desktop.ksx" > "$OUT/$p-desktop.out" 2> "$OUT/$p-desktop.err"
  rc=$?
  want=$(expected "$p" desktop)
  # no desktop não há GPIO: o blink só confere que detecta isso pelo CAPS
  [ "$p" = blink ] && want="blink: sem GPIO"
  if [ $rc -ne 0 ] || ! grep -qF "$want" "$OUT/$p-desktop.out"; then
    note_fail "$p/desktop: rc=$rc, faltou '$want': $(head -3 "$OUT/$p-desktop.out" "$OUT/$p-desktop.err")"
  else
    echo "  PASS $p ($(head -1 "$OUT/$p-desktop.out"))"
  fi
done
if $CC32 -m32 -std=c11 -O1 -Wall -Itest -o "$OUT/selftest.gcc" test/embedded/selftest.c 2> "$OUT/selftest.gcc.cerr" \
   && [ ! -s "$OUT/selftest.gcc.cerr" ] && "./$OUT/selftest.gcc" > "$OUT/selftest.gcc.out" \
   && cmp -s "$OUT/selftest.gcc.out" "$OUT/selftest-desktop.out"; then
  echo "  PASS selftest = gcc -m32"
else
  note_fail "selftest: gcc -m32 diverge ou avisa: $(head -3 "$OUT/selftest.gcc.cerr" "$OUT/selftest.gcc.out")"
fi

# --- 2. firmware --------------------------------------------------------------
# $1 plataforma (AVR/ARM), $2 alvo make, $3 elf gerado, $4 comando de size,
# resto: simulador
run_platform() {
  local plat=$1 target=$2 elf=$3 sizecmd=$4
  shift 4
  echo "== $plat ($*) =="
  for p in $PROGS; do
    if ! build "$p" "$plat"; then
      note_fail "$p/$plat: kscc nao compilou: $(head -3 "$OUT/$p-$plat.cerr")"
      continue
    fi
    local ksx="$ROOT/$OUT/$p-$plat.ksx"
    if ! make -s -C "$KSVM2_ABS" "$target" KSX="$ksx" > "$OUT/$p-$plat.make" 2>&1; then
      note_fail "$p/$plat: make $target falhou: $(tail -3 "$OUT/$p-$plat.make")"
      continue
    fi
    cp "$KSVM2_ABS/$elf" "$OUT/$p-$plat.elf"
    local ksx_size flash
    ksx_size=$(stat -c %s "$ksx")
    flash=$($sizecmd "$OUT/$p-$plat.elf" | awk 'NR == 2 { print $1 + $2 }')
    if $EXPECT "$p-$plat" "$OUT/$p-$plat.log" 60 "$(expected "$p" "$plat")" "KSVM halt 00000000" \
         -- "$@" "$OUT/$p-$plat.elf"; then
      :
    else
      note_fail "$p/$plat: UART sem a saida esperada (log em $OUT/$p-$plat.log)"
    fi
    local uart
    uart=$(grep -v '^$' "$OUT/$p-$plat.log" | grep -v 'Loaded\|^SIMAVR\|^avr_\|firmware' | head -3 | tr '\n' '|')
    report+=("$(printf '%-9s %-4s ksx=%6d B  firmware(flash)=%6d B  uart: %s' "$p" "$plat" "$ksx_size" "$flash" "$uart")")
    if [ "$plat" = AVR ] && [ "$flash" -gt $AVR_FLASH ]; then
      note_fail "$p/AVR: firmware de $flash B nao cabe nos 32 KB do ATmega328P"
    fi
  done
}

if command -v avr-gcc > /dev/null && command -v simavr > /dev/null; then
  run_platform AVR avr-custom build/ports/avr/avr-custom.elf avr-size \
    simavr -m atmega328p -f 16000000
else
  echo "AVR: avr-gcc/simavr nao encontrados, pulando"
fi
if command -v arm-none-eabi-gcc > /dev/null && command -v qemu-system-arm > /dev/null; then
  run_platform ARM arm-custom build/ports/arm/arm-custom.elf arm-none-eabi-size \
    qemu-system-arm -M mps2-an385 -nographic -semihosting -kernel
else
  echo "ARM: arm-none-eabi-gcc/qemu-system-arm nao encontrados, pulando"
fi

echo
echo "== tamanhos (ATmega328P: flash de $AVR_FLASH B, incluindo a VM) =="
for r in "${report[@]}"; do echo "$r"; done
echo
if [ $fail -ne 0 ]; then
  red "embarcado: FALHAS: ${failed[*]}"
  exit 1
fi
echo "embarcado: OK"
