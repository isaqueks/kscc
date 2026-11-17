// <ksvm/gpio.h> — pinos de E/S (grupo 4 de syscalls, doc 05 §2).
//
// Todas devolvem -1 (errno = ENOSYS) se a HAL não tem GPIO, como no
// runner de desktop. Não dá para perguntar antes com ks_sys_caps: a VM
// responde 0 para todas as syscalls encaminhadas à HAL, inclusive as que
// existem (ksvm2, src/core/syscall.c). A numeração dos pinos é a da porta
// (no AVR, a do Arduino Uno).
#ifndef __KSVM_GPIO_H
#define __KSVM_GPIO_H

#include <ksvm/syscall.h>

#define KS_LOW 0
#define KS_HIGH 1

// mode: KS_PIN_INPUT, KS_PIN_OUTPUT ou KS_PIN_INPUT_PULLUP
static inline int ks_pin_mode(int pin, int mode) {
  return ks_sys_pin_mode(pin, mode);
}

static inline int ks_pin_write(int pin, int value) {
  return ks_sys_pin_write(pin, value);
}

// 0 ou 1, ou -1
static inline int ks_pin_read(int pin) {
  return ks_sys_pin_read(pin);
}

// 0..1023, ou -1
static inline int ks_analog_read(int channel) {
  return ks_sys_analog_read(channel);
}

// duty 0..255
static inline int ks_pwm_write(int pin, int duty) {
  return ks_sys_pwm_write(pin, duty);
}

#endif
