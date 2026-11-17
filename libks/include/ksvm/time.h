// <ksvm/time.h> — tempo da KSVM (doc 05 §2). Não há relógio de
// calendário: só milissegundos desde o boot, com volta em 2^32.
#ifndef __KSVM_TIME_H
#define __KSVM_TIME_H

#include <ksvm/syscall.h>

// Milissegundos desde o boot. Compare intervalos com subtração sem sinal
// (`ks_millis() - t0 >= ms`), que continua certa depois da volta.
static inline unsigned ks_millis(void) {
  return ks_sys_millis();
}

// Dorme `ms` milissegundos (no MCU, em modo de baixo consumo; num host
// sem sleep, devolve o controle ao host com YIELD).
static inline void ks_sleep_ms(unsigned ms) {
  ks_sys_sleep_ms(ms);
}

#endif
