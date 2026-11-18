// switch sobre um float convertido para int é válido; case com cast de
// float numa expressão constante inteira também (C11 6.6p6).
#include "float_test.h"
int main() {
  float f = rt(2.75f);
  int r = 0;
  switch ((int)f) {
  case (int)1.5: r = 1; break;
  case (int)2.9f: r = 2; break;
  default: r = 3;
  }
  EQ(2, r);
  return 0;
}
