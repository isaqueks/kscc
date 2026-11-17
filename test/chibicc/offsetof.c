#include "test.h"
#include <stddef.h>

typedef struct {
  int a;
  char b;
  int c;
#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
  double d;
#endif
} T;

int main() {
  ASSERT(0, offsetof(T, a));
  ASSERT(4, offsetof(T, b));
  ASSERT(8, offsetof(T, c));
#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
  ASSERT(12, offsetof(T, d)); // kscc: double tem alinhamento 4 (doc 02 §1)
#endif

  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
