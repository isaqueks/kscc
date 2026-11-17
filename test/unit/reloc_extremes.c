// Inicializadores globais `.word sym ± off` com deslocamentos nos extremos
// de 32 bits. emit_reloc negava um int32_t igual a INT32_MIN (UB no
// compilador, pego pelo UBSan em make test-asan).
#include "minirt.h"
char buf[16];
char *p1 = buf - 5;
char *p2 = buf + 0x7fffffff;
char *p3 = buf + 0x80000000u;
char *p4 = buf - 0x7fffffff - 1;
char *p5 = &buf[3] - 0xffffffffu;
int main() {
  EQ(-5, p1 - buf);
  EQ(0x7fffffff, (unsigned)(p2 - buf));
  EQ(0x80000000, (unsigned)(p3 - buf));
  EQ(0x80000000, (unsigned)(p4 - buf));
  EQ(4, p5 - buf);
  return 0;
}
