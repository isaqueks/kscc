// Saída por syscall PUTS/PUTC via minirt; expect-out com várias linhas.
#include "minirt.h"
// expect-out: hello, ksvm
// expect-out: -123 4294967295 0xdeadbeef
// expect-out: -2147483648
int main() {
  ks_put_str("hello, ksvm\n");
  ks_put_int(-123); ks_sys_putc(' ');
  ks_put_uint(4294967295u); ks_sys_putc(' ');
  ks_put_hex(0xdeadbeef); NL();
  ks_put_int(-2147483647 - 1); NL();
  return 0;
}
