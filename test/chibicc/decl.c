#include "test.h"

int main() {
  ASSERT(1, ({ char x; sizeof(x); }));
  ASSERT(2, ({ short int x; sizeof(x); }));
  ASSERT(2, ({ int short x; sizeof(x); }));
  ASSERT(4, ({ int x; sizeof(x); }));
  ASSERT(4, ({ long int x; sizeof(x); })); // kscc: long tem 4 bytes (ILP32)
  ASSERT(4, ({ int long x; sizeof(x); })); // kscc: idem

#ifdef KSCC_HAS_LL // kscc: long long só na fase 5
  ASSERT(8, ({ long long x; sizeof(x); }));
#endif

  ASSERT(0, ({ _Bool x=0; x; }));
  ASSERT(1, ({ _Bool x=1; x; }));
  ASSERT(1, ({ _Bool x=2; x; }));
  ASSERT(1, (_Bool)1);
  ASSERT(1, (_Bool)2);
  ASSERT(0, (_Bool)(char)256);

  printf("OK\n");
  return 0;
}
