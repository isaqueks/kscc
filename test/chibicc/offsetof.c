#include "test.h"
#include <stddef.h>

typedef struct {
  int a;
  char b;
  int c;
  double d;
} T;

int main() {
  ASSERT(0, offsetof(T, a));
  ASSERT(4, offsetof(T, b));
  ASSERT(8, offsetof(T, c));
  ASSERT(12, offsetof(T, d)); // kscc: double tem alinhamento 4 (doc 02 §1)

  printf("OK\n");
  return 0;
}
