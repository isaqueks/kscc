#include "test.h"

#ifdef KSCC_HAS_FLOAT // kscc: ponto flutuante só na fase 6
float g40 = 1.5;
double g41 = 0.0 ? 55 : (0, 1 + 1 * 5.0 / 2 * (double)2 * (int)2.0);
#endif

int main() {
  ASSERT(10, ({ enum { ten=1+2+3+4 }; ten; }));
  ASSERT(1, ({ int i=0; switch(3) { case 5-2+0*3: i++; } i; }));
  ASSERT(8, ({ int x[1+1]; sizeof(x); }));
  ASSERT(6, ({ char x[8-2]; sizeof(x); }));
  ASSERT(6, ({ char x[2*3]; sizeof(x); }));
  ASSERT(3, ({ char x[12/4]; sizeof(x); }));
  ASSERT(2, ({ char x[12%10]; sizeof(x); }));
  ASSERT(0b100, ({ char x[0b110&0b101]; sizeof(x); }));
  ASSERT(0b111, ({ char x[0b110|0b101]; sizeof(x); }));
  ASSERT(0b110, ({ char x[0b111^0b001]; sizeof(x); }));
  ASSERT(4, ({ char x[1<<2]; sizeof(x); }));
  ASSERT(2, ({ char x[4>>1]; sizeof(x); }));
  ASSERT(2, ({ char x[(1==1)+1]; sizeof(x); }));
  ASSERT(1, ({ char x[(1!=1)+1]; sizeof(x); }));
  ASSERT(1, ({ char x[(1<1)+1]; sizeof(x); }));
  ASSERT(2, ({ char x[(1<=1)+1]; sizeof(x); }));
  ASSERT(2, ({ char x[1?2:3]; sizeof(x); }));
  ASSERT(3, ({ char x[0?2:3]; sizeof(x); }));
  ASSERT(3, ({ char x[(1,3)]; sizeof(x); }));
  ASSERT(2, ({ char x[!0+1]; sizeof(x); }));
  ASSERT(1, ({ char x[!1+1]; sizeof(x); }));
  ASSERT(2, ({ char x[~-3]; sizeof(x); }));
  ASSERT(2, ({ char x[(5||6)+1]; sizeof(x); }));
  ASSERT(1, ({ char x[(0||0)+1]; sizeof(x); }));
  ASSERT(2, ({ char x[(1&&1)+1]; sizeof(x); }));
  ASSERT(1, ({ char x[(1&&0)+1]; sizeof(x); }));
  ASSERT(3, ({ char x[(int)3]; sizeof(x); }));
  ASSERT(15, ({ char x[(char)0xffffff0f]; sizeof(x); }));
  ASSERT(0x10f, ({ char x[(short)0xffff010f]; sizeof(x); }));
#ifdef KSCC_HAS_LL // kscc: 0xfffffffffff só cabe em long long (fase 5)
  ASSERT(4, ({ char x[(int)0xfffffffffff+5]; sizeof(x); }));
#endif
  ASSERT(4, ({ char x[(int)0xffffffff+5]; sizeof(x); })); // kscc: versão de 32 bits
  ASSERT(8, ({ char x[(int*)0+2]; sizeof(x); }));
  ASSERT(12, ({ char x[(int*)16-1]; sizeof(x); }));
  ASSERT(3, ({ char x[(int*)16-(int*)4]; sizeof(x); }));

  ASSERT(4, ({ char x[(-1>>31)+5]; sizeof(x); }));
  ASSERT(255, ({ char x[(unsigned char)0xffffffff]; sizeof(x); }));
  ASSERT(0x800f, ({ char x[(unsigned short)0xffff800f]; sizeof(x); }));
#ifdef KSCC_HAS_LL // kscc: constantes e shifts de 64 bits (fase 5)
  ASSERT(1, ({ char x[(unsigned int)0xfffffffffff>>31]; sizeof(x); }));
  ASSERT(1, ({ char x[(long)-1/((long)1<<62)+1]; sizeof(x); }));
  ASSERT(4, ({ char x[(unsigned long)-1/((long)1<<62)+1]; sizeof(x); }));
#endif
  // kscc: versões de 32 bits dos três casos acima
  ASSERT(1, ({ char x[(unsigned int)0xffffffff>>31]; sizeof(x); }));
  ASSERT(1, ({ char x[(long)-1/((long)1<<30)+1]; sizeof(x); }));
  ASSERT(4, ({ char x[(unsigned long)-1/((long)1<<30)+1]; sizeof(x); }));
  ASSERT(1, ({ char x[(unsigned)1<-1]; sizeof(x); }));
  ASSERT(1, ({ char x[(unsigned)1<=-1]; sizeof(x); }));

#ifdef KSCC_HAS_FLOAT
  ASSERT(1, g40==1.5);
  ASSERT(1, g41==11);
#endif

  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
