// errno depois de uma syscall que o host não tem (GPIO no runner de
// desktop: -1 e ENOSYS), sucesso não zera errno, e os wrappers de
// syscall do grupo 0, 2 e 5 devolvem valores plausíveis.
#include <errno.h>
#include <ksvm/gpio.h>
#include <ksvm/syscall.h>
#include <ksvm/time.h>
#include "minirt.h"

int main() {
  errno = 0;
  EQ(-1, ks_pin_mode(13, KS_PIN_OUTPUT));
  EQ(ENOSYS, errno);
  errno = 0;
  EQ(-1, ks_pin_write(13, KS_HIGH));
  EQ(ENOSYS, errno);
  errno = 0;
  EQ(-1, ks_analog_read(0));
  EQ(ENOSYS, errno);
  EQ(-1, ks_pin_read(2));
  EQ(-1, ks_pwm_write(3, 128));
  EQ(ENOSYS, errno);

  // Sucesso não zera errno (C11 7.5p3)
  errno = EINVAL;
  EQ(0, ks_sys_putc('.'));
  EQ(EINVAL, errno);
  ks_sys_putc('\n');

  // CAPS: o núcleo conhece PUTC; GPIO é encaminhado à HAL e responde 0
  errno = 0;
  EQ(1, ks_sys_caps(KS_SYS_PUTC));
  EQ(1, ks_sys_caps(KS_SYS_MEM_INFO));
  EQ(0, ks_sys_caps(KS_SYS_PIN_MODE));
  EQ(0, ks_sys_caps(0x3FFF));
  EQ(0, errno);

  // VERSION: major 2
  EQ(2, ks_sys_version() >> 16);

  // RANDOM muda a cada chamada
  unsigned r1 = ks_sys_random(), r2 = ks_sys_random();
  EQ(1, r1 != r2);

  // MILLIS não volta para trás; sleep de 1 ms
  unsigned t0 = ks_millis();
  ks_sleep_ms(1);
  EQ(1, ks_millis() - t0 < 1000000u);

  // MEM_INFO: heap entre o fim do .bss e o limite da pilha, dentro da RAM
  struct ks_mem_info mi;
  errno = 0;
  ks_sys_mem_info(&mi);
  EQ(0, errno);  // r2 (tamanho da RAM) não é erro
  EQ(1, mi.ram_size > 0);
  EQ(1, mi.heap_start >= mi.ram_base);
  EQ(1, mi.heap_start <= mi.stack_limit);
  EQ(1, mi.stack_limit < mi.ram_base + mi.ram_size);
  EQ(1, (unsigned)&mi > mi.stack_limit);  // esta local está na pilha
  return 0;
}
// expect-out: .
