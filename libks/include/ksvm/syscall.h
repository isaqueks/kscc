// <ksvm/syscall.h> — acesso direto às syscalls da KSVM (doc 05 §1).
//
// Cada função é um wrapper de libks/asm/syscall.ksasm: `syscall N` e
// retorno em r1. Em erro devolvem -1 e põem o código KSVM_ERR_* em errno
// (ks_errno; os nomes estão em <errno.h>). Sucesso não zera errno.
#ifndef __KSVM_SYSCALL_H
#define __KSVM_SYSCALL_H

// Números das syscalls (doc 06 §3 da ksvm2), para ks_sys_caps().
#define KS_SYS_EXIT 0x00
#define KS_SYS_CAPS 0x01
#define KS_SYS_VERSION 0x02
#define KS_SYS_YIELD 0x03
#define KS_SYS_ABORT 0x04
#define KS_SYS_PUTC 0x10
#define KS_SYS_GETC 0x11
#define KS_SYS_PUTS 0x12
#define KS_SYS_GETS 0x13
#define KS_SYS_MILLIS 0x20
#define KS_SYS_SLEEP_MS 0x21
#define KS_SYS_MICROS 0x22
#define KS_SYS_OPEN 0x30
#define KS_SYS_CLOSE 0x31
#define KS_SYS_READ 0x32
#define KS_SYS_WRITE 0x33
#define KS_SYS_SEEK 0x34
#define KS_SYS_PIN_MODE 0x40
#define KS_SYS_PIN_WRITE 0x41
#define KS_SYS_PIN_READ 0x42
#define KS_SYS_ANALOG_READ 0x43
#define KS_SYS_PWM_WRITE 0x44
#define KS_SYS_MEM_INFO 0x50
#define KS_SYS_RANDOM 0x51

// Flags de ks_sys_open (grupo 3, só no desktop).
#define KS_O_RDONLY 1
#define KS_O_WRONLY 2
#define KS_O_CREAT 4
#define KS_O_APPEND 8

// Modos de ks_sys_pin_mode (grupo 4).
#define KS_PIN_INPUT 0
#define KS_PIN_OUTPUT 1
#define KS_PIN_INPUT_PULLUP 2

// Layout da RAM devolvido por MEM_INFO; a ordem dos campos é a dos
// registradores r1-r4 da syscall.
struct ks_mem_info {
  unsigned ram_base;
  unsigned ram_size;
  unsigned heap_start;   // fim do .bss: o heap começa aqui
  unsigned stack_limit;  // menor endereço que a pilha pode alcançar
};

// Grupo 0 — núcleo
_Noreturn void ks_sys_exit(int code);
int ks_sys_caps(int num);  // só conhece as syscalls do núcleo da VM
unsigned ks_sys_version(void);
void ks_sys_yield(void);
_Noreturn void ks_sys_abort(int code);

// Grupo 1 — console
int ks_sys_putc(int c);
int ks_sys_getc(void);  // não bloqueia: -1 e errno = EAGAIN sem dado
int ks_sys_puts(const char *p, unsigned len);
int ks_sys_gets(char *p, unsigned max);

// Grupo 2 — tempo
unsigned ks_sys_millis(void);
int ks_sys_sleep_ms(unsigned ms);
unsigned ks_sys_micros(void);

// Grupo 3 — arquivos (desktop; fd 0/1/2 = stdin/stdout/stderr)
int ks_sys_open(const char *path, unsigned len, int flags);
int ks_sys_close(int fd);
int ks_sys_read(int fd, void *p, unsigned len);
int ks_sys_write(int fd, const void *p, unsigned len);
int ks_sys_seek(int fd, int offset, int whence);

// Grupo 4 — GPIO (MCU)
int ks_sys_pin_mode(int pin, int mode);
int ks_sys_pin_write(int pin, int value);
int ks_sys_pin_read(int pin);
int ks_sys_analog_read(int channel);
int ks_sys_pwm_write(int pin, int duty);

// Grupo 5 — memória
void ks_sys_mem_info(struct ks_mem_info *mi);
unsigned ks_sys_random(void);

#endif
