#include "test.h"

int _Alignas(512) g1;
int _Alignas(512) g2;
char g3;
int g4;
long g5;
char g6;

int main() {
  ASSERT(1, _Alignof(char));
  ASSERT(2, _Alignof(short));
  ASSERT(4, _Alignof(int));
  ASSERT(4, _Alignof(long)); // kscc: long tem 4 bytes (ILP32)
#ifdef KSCC_HAS_LL // kscc: long long só na fase 5 (e terá alinhamento 4, doc 02 §1)
  ASSERT(8, _Alignof(long long));
#endif
  ASSERT(1, _Alignof(char[3]));
  ASSERT(4, _Alignof(int[3]));
  ASSERT(1, _Alignof(struct {char a; char b;}[2]));
  ASSERT(4, _Alignof(struct {char a; long b;}[2])); // kscc: long tem 4 bytes (ILP32)

  ASSERT(1, ({ _Alignas(char) char x, y; &y-&x; }));
  ASSERT(4, ({ _Alignas(long) char x, y; &y-&x; })); // kscc: long tem 4 bytes (ILP32)
  ASSERT(32, ({ _Alignas(32) char x, y; &y-&x; }));
  ASSERT(32, ({ _Alignas(32) int *x, *y; ((char *)&y)-((char *)&x); }));
  ASSERT(16, ({ struct { _Alignas(16) char x, y; } a; &a.y-&a.x; }));
  ASSERT(8, ({ struct T { _Alignas(8) char a; }; _Alignof(struct T); }));

  ASSERT(0, (long)(char *)&g1 % 512);
  ASSERT(0, (long)(char *)&g2 % 512);
  ASSERT(0, (long)(char *)&g4 % 4);
  ASSERT(0, (long)(char *)&g5 % 4); // kscc: long tem 4 bytes (ILP32)

  ASSERT(1, ({ char x; _Alignof(x); }));
  ASSERT(4, ({ int x; _Alignof(x); }));
  ASSERT(1, ({ char x; _Alignof x; }));
  ASSERT(4, ({ int x; _Alignof x; }));

  ASSERT(1, _Alignof(char) << 31 >> 31);
  ASSERT(1, _Alignof(char) << 63 >> 63);
  ASSERT(1, ({ char x; _Alignof(x) << 63 >> 63; }));

#if 0 // kscc: regra do psABI x86-64 (arrays de 16 bytes ou mais alinhados a 16); na KSVM o alinhamento de char[N] é 1
  ASSERT(0, ({ char x[16]; (unsigned long)&x % 16; }));
  ASSERT(0, ({ char x[17]; (unsigned long)&x % 16; }));
  ASSERT(0, ({ char x[100]; (unsigned long)&x % 16; }));
  ASSERT(0, ({ char x[101]; (unsigned long)&x % 16; }));
#endif
  // kscc: o equivalente na KSVM com _Alignas, que o kscc honra em locais
  ASSERT(0, ({ _Alignas(16) char x[17]; (unsigned long)&x % 16; }));
  ASSERT(0, ({ char c; _Alignas(16) char x[101]; c = 1; (unsigned long)&x % 16; }));

  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
