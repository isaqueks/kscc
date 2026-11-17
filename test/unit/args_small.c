// Argumentos char/short (em registrador e na pilha) e retorno char/short.
#include "minirt.h"
int mix(char a, unsigned char b, short c, unsigned short d,
        char e, unsigned char f, short g, unsigned short h) {
  return a + b + c + d + e + f + g + h;
}
char ret_char(int x) { return x; }
unsigned char ret_uchar(int x) { return x; }
short ret_short(int x) { return x; }
_Bool ret_bool(int x) { return x; }
int main() {
  EQ(-1 + 255 - 1 + 65535 - 1 + 255 - 1 + 65535, mix(-1, 255, -1, 65535, -1, 255, -1, 65535));
  EQ(-1 + 255 - 1 + 65535 - 1 + 255 - 1 + 65535, mix(255, -1, 65535, -1, 255, -1, 65535, -1));
  EQ(44, ret_char(300));
  EQ(-56, ret_char(200));
  EQ(200, ret_uchar(200));
  EQ(-1, ret_short(65535));
  EQ(1, ret_bool(256));
  return 0;
}
