// Literais de string em .rodata; escrita neles é FAULT_MEM_PERM.
#include "minirt.h"
// expect-out: abc|xyz
int main() {
  char *s = "abc";
  ks_put_str(s);
  ks_sys_putc('|');
  ks_sys_puts("xyz\n", 4);
  EQ('c', s[2]);
  EQ(4, sizeof("abc"));
  EQ(0, "abc"[3]);
  return 0;
}
