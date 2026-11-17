#include "test.h"

int main() {
  ASSERT(1, sizeof(char));
  ASSERT(2, sizeof(short));
  ASSERT(2, sizeof(short int));
  ASSERT(2, sizeof(int short));
  ASSERT(4, sizeof(int));
  ASSERT(4, sizeof(long)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(long int)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(long int)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(char *)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(int *)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(long *)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(int **)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(int(*)[4])); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(16, sizeof(int*[4])); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(16, sizeof(int[4]));
  ASSERT(48, sizeof(int[3][4]));
  ASSERT(8, sizeof(struct {int a; int b;}));

  ASSERT(4, sizeof(-10 + (long)5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(-10 - (long)5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(-10 * (long)5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(-10 / (long)5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof((long)-10 + 5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof((long)-10 - 5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof((long)-10 * 5)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof((long)-10 / 5)); // kscc: ILP32: long e ponteiros têm 4 bytes

  ASSERT(1, ({ char i; sizeof(++i); }));
  ASSERT(1, ({ char i; sizeof(i++); }));

  ASSERT(4, sizeof(int(*)[10])); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(int(*)[][10])); // kscc: ILP32: long e ponteiros têm 4 bytes

  ASSERT(4, sizeof(struct { int x, y[]; }));

  ASSERT(1, sizeof(char));
  ASSERT(1, sizeof(signed char));
  ASSERT(1, sizeof(signed char signed));
  ASSERT(1, sizeof(unsigned char));
  ASSERT(1, sizeof(unsigned char unsigned));

  ASSERT(2, sizeof(short));
  ASSERT(2, sizeof(int short));
  ASSERT(2, sizeof(short int));
  ASSERT(2, sizeof(signed short));
  ASSERT(2, sizeof(int short signed));
  ASSERT(2, sizeof(unsigned short));
  ASSERT(2, sizeof(int short unsigned));

  ASSERT(4, sizeof(int));
  ASSERT(4, sizeof(signed int));
  ASSERT(4, sizeof(signed));
  ASSERT(4, sizeof(signed signed));
  ASSERT(4, sizeof(unsigned int));
  ASSERT(4, sizeof(unsigned));
  ASSERT(4, sizeof(unsigned unsigned));

  ASSERT(4, sizeof(long)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(signed long)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(signed long int)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(unsigned long)); // kscc: ILP32: long e ponteiros têm 4 bytes
  ASSERT(4, sizeof(unsigned long int)); // kscc: ILP32: long e ponteiros têm 4 bytes

#ifdef KSCC_HAS_LL // kscc: long long só na fase 5
  ASSERT(8, sizeof(long long));
  ASSERT(8, sizeof(signed long long));
  ASSERT(8, sizeof(signed long long int));
  ASSERT(8, sizeof(unsigned long long));
  ASSERT(8, sizeof(unsigned long long int));
#endif

  ASSERT(1, sizeof((char)1));
  ASSERT(2, sizeof((short)1));
  ASSERT(4, sizeof((int)1));
  ASSERT(4, sizeof((long)1)); // kscc: ILP32: long e ponteiros têm 4 bytes

  ASSERT(4, sizeof((char)1 + (char)1));
  ASSERT(4, sizeof((short)1 + (short)1));
  ASSERT(4, sizeof(1?2:3));
  ASSERT(4, sizeof(1?(short)2:(char)3));
  ASSERT(4, sizeof(1?(long)2:(char)3)); // kscc: ILP32: long e ponteiros têm 4 bytes

  ASSERT(1, sizeof(char) << 31 >> 31);
  ASSERT(1, sizeof(char) << 63 >> 63);

#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
  ASSERT(4, sizeof(float));
  ASSERT(8, sizeof(double));

  ASSERT(4, sizeof(1f+2));
  ASSERT(8, sizeof(1.0+2));
  ASSERT(4, sizeof(1f-2));
  ASSERT(8, sizeof(1.0-2));
  ASSERT(4, sizeof(1f*2));
  ASSERT(8, sizeof(1.0*2));
  ASSERT(4, sizeof(1f/2));
  ASSERT(8, sizeof(1.0/2));

  ASSERT(16, sizeof(long double));
#endif

  ASSERT(1, sizeof(main));

  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
