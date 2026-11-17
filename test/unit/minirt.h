// minirt.h — protótipos do runtime mínimo (lib/minirt.ksasm) usado pelos
// testes unitários das fases 0–3. Substituído pela libks na fase 4.
#ifndef KSCC_MINIRT_H
#define KSCC_MINIRT_H

void ks_sys_exit(int code);
int ks_sys_putc(int c);
int ks_sys_puts(const char *p, int len);
void ks_put_str(const char *s);
void ks_put_int(int v);
void ks_put_uint(unsigned v);
void ks_put_hex(unsigned v);
void ks_assert_eq(int expected, int actual, int line);

#define EQ(expected, actual) ks_assert_eq((expected), (actual), __LINE__)
#define NL() ks_sys_putc('\n')

#endif
