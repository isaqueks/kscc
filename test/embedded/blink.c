// test/embedded/blink.c — pisca o pino 13 (LED do Uno; PB5 no ATmega328P)
// três vezes com ks_sys_pin_mode/ks_sys_pin_write/ks_sys_sleep_ms e imprime
// "blink ok". Também lê o pino de volta (no simavr dá 101010; o QEMU não
// modela o GPIO do AN385 e lê 0) e confere que o tempo passou (MILLIS).
#include "kstest.h"

int ks_sys_caps(int n);
int ks_sys_pin_mode(int pin, int mode);
int ks_sys_pin_write(int pin, int value);
int ks_sys_pin_read(int pin);
int ks_sys_sleep_ms(unsigned ms);
unsigned ks_sys_millis(void);

#define LED 13
#define HALF_PERIOD_MS 50

int main(void) {
  // CAPS só conhece as syscalls do núcleo da VM: o grupo 4 é encaminhado
  // ao HAL e CAPS responde 0 mesmo no AVR/ARM (ksvm2 src/core/syscall.c).
  // A detecção de GPIO é pelo retorno de PIN_MODE (-1/NOSYS no desktop).
  ks_put_str("blink caps(PIN_MODE)=");
  ks_put_int(ks_sys_caps(0x40));
  ks_put_chr('\n');
  if (ks_sys_pin_mode(LED, 1) != 0) {
    ks_put_str("blink: sem GPIO nesta plataforma\n");
    return 0;
  }
  int errors = 0;
  unsigned t0 = ks_sys_millis();
  char levels[7];
  for (int i = 0; i < 3; i++) {
    errors += ks_sys_pin_write(LED, 1) != 0;
    levels[2 * i] = (char)('0' + ks_sys_pin_read(LED));
    ks_sys_sleep_ms(HALF_PERIOD_MS);
    errors += ks_sys_pin_write(LED, 0) != 0;
    levels[2 * i + 1] = (char)('0' + ks_sys_pin_read(LED));
    ks_sys_sleep_ms(HALF_PERIOD_MS);
  }
  levels[6] = 0;
  unsigned elapsed = ks_sys_millis() - t0;
  ks_put_str("blink levels ");
  ks_put_str(levels);
  ks_put_chr('\n');
  if (errors == 0 && elapsed >= 6 * HALF_PERIOD_MS) {
    ks_put_str("blink ok\n");
    return 0;
  }
  ks_put_str("blink FALHOU: erros=");
  ks_put_int(errors);
  ks_put_str(" ms=");
  ks_put_uint(elapsed);
  ks_put_chr('\n');
  return 1;
}
