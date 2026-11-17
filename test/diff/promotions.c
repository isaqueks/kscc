// Promoções de char/short, signed/unsigned char, conversões de retorno e
// de argumento, compostos (+=, <<=) em tipos pequenos, incremento e wrap.
#include "kstest.h"

static signed char sc_id(signed char c) { return c; }
static unsigned char uc_id(unsigned char c) { return c; }
static short s_id(short s) { return s; }
static unsigned short us_id(unsigned short s) { return s; }
static char ret_char(int x) { return (char)x; }
static int take_char(char c) { return c; }

int main(void) {
  char c = 'A';
  signed char sc = -128;
  unsigned char uc = 255;
  short s = -32768;
  unsigned short us = 65535;
  ks_put_kv("c+1", c + 1);
  ks_put_kv("sc-1 (int)", sc - 1);
  ks_put_kv("uc+1 (int)", uc + 1);
  ks_put_kv("s-1 (int)", s - 1);
  ks_put_kv("us+1 (int)", us + 1);
  ks_put_kv("sizeof(c+c)", (int)sizeof(c + c));
  ks_put_kv("sizeof(+uc)", (int)sizeof(+uc));
  ks_put_kv("sizeof(us)", (int)sizeof(us));
  // atribuição composta com truncamento
  uc += 10;
  ks_put_kv("uc+=10", uc);
  sc -= 1;
  ks_put_kv("sc-=1", sc);
  us <<= 4;
  ks_put_kv("us<<=4", us);
  s = (short)(s * 3); // -98304 truncado
  ks_put_kv("s*=3", s);
  char cc = 127;
  cc++;
  ks_put_kv("cc++", cc);
  unsigned char ucc = 0;
  ucc--;
  ks_put_kv("ucc--", ucc);
  // argumentos e retornos convertidos
  ks_put_kv("sc_id(300)", sc_id((signed char)300));
  ks_put_kv("uc_id(-1)", uc_id((unsigned char)-1));
  ks_put_kv("s_id(70000)", s_id((short)70000));
  ks_put_kv("us_id(-2)", us_id((unsigned short)-2));
  ks_put_kv("ret_char(0x1FF)", ret_char(0x1FF));
  ks_put_kv("take_char(200)", take_char((char)200));
  // comparações entre tipos pequenos
  signed char m1 = -1;
  unsigned char ff = 0xFF;
  ks_put_kv("m1 == ff", m1 == ff);
  ks_put_kv("(unsigned char)m1 == ff", (unsigned char)m1 == ff);
  unsigned short ushort_ff = 0xFFFF;
  short sh_m1 = -1;
  ks_put_kv("sh_m1 == ushort_ff", sh_m1 == ushort_ff);
  ks_put_kv("sh_m1 < 0u", sh_m1 < 0u);
  // arrays de char com valores altos e soma
  char text[] = "\x80\xff\x7f\x01";
  int sum = 0;
  unsigned usum = 0;
  for (int i = 0; i < 4; i++) {
    sum += text[i];
    usum += (unsigned char)text[i];
  }
  ks_put_kv("sum", sum);
  ks_put_kv("usum", (int)usum);
  // caracteres como índices e em switch
  int counts[256] = {0};
  const char *msg = "promotions are subtle: \xe9\xe9!";
  for (const char *p = msg; *p; p++)
    counts[(unsigned char)*p]++;
  ks_put_kv("count e", counts['e']);
  ks_put_kv("count 0xe9", counts[0xe9]);
  ks_put_kv("count space", counts[' ']);
  // ~ e shift em tipos pequenos (promovidos a int)
  unsigned char b = 0x0F;
  ks_put_kv("~b", ~b);
  ks_put_kv("(unsigned char)~b", (unsigned char)~b);
  ks_put_kv("b << 28 >> 28", (int)((unsigned)b << 28) >> 28);
  short hs = 0x4000;
  ks_put_kv("hs << 2", hs << 2);
  ks_put_kv("(short)(hs << 2)", (short)(hs << 2));
  // ternário com tipos mistos
  int flag = 1;
  ks_put_kv("flag ? uc : sc", flag ? uc : sc);
  ks_put_kv("!flag ? uc : sc", !flag ? uc : sc);
  ks_put_kv("flag ? -1 : 0u > 0", (flag ? -1 : 0u) > 0);
  _Bool bo = 256;
  ks_put_kv("(_Bool)256", bo);
  bo = flag == 1;
  ks_put_kv("bool from cmp", bo);
  return 0;
}
