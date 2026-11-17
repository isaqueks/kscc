#include "kstest.h"
// Enumeradores acima de INT_MAX (extensão do gcc, sem aviso em -Wall):
// o enum e essas constantes são unsigned int. Antes o valor virava um
// int negativo em silêncio (F31 > 0 era falso, F31 >> 1 negativo).
enum E { F31 = 1u << 31, F0 = 1 };
enum G { BIG = 0xFFFFFFFF };
enum H { MAXI = 0x7fffffff, NEXT };
int main(void) {
  enum E e = F31;
  ks_put_kv("F31 > 0", F31 > 0);
  ks_put_kv("F31 >> 1", (int)(F31 >> 1));
  ks_put_kv("F31 / 2", (int)(F31 / 2));
  ks_put_kv("e > 0", e > 0);
  ks_put_kv("BIG > 0", BIG > 0);
  ks_put_kv("NEXT > MAXI", NEXT > MAXI);
  ks_put_kv("F0 - 2 < 0", F0 - 2 < 0);
  ks_put_kv("sizeof", (int)sizeof(enum E));
  return 0;
}
