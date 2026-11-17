// case com intervalo GNU, valores grandes (cmpl), char e unsigned.
#include "minirt.h"
int f(int x) {
  switch (x) {
  case 0 ... 9: return 1;
  case 97 ... 122: return 2;
  case 100000 ... 200000: return 3;
  case -2147483647 - 1: return 4;
  case 0x7FFFFFFF: return 5;
  case 50000: return 6;
  }
  return 0;
}
int g(unsigned u) {
  switch (u) {
  case 0xFFFFFFFF: return 1;
  case 0x80000000 ... 0x80000010: return 2;
  }
  return 0;
}
int h(char c) {
  switch (c) {
  case -1: return 1;
  case 200: return 2;          // nunca: char é signed
  }
  return 0;
}
int main() {
  EQ(1, f(0)); EQ(1, f(9)); EQ(0, f(10)); EQ(0, f(-1));
  EQ(2, f(109)); EQ(3, f(150000)); EQ(0, f(200001));
  EQ(4, f(-2147483647 - 1)); EQ(5, f(0x7FFFFFFF)); EQ(6, f(50000));
  EQ(1, g(-1)); EQ(2, g(0x80000008u)); EQ(0, g(0x7FFFFFFF));
  EQ(1, h(255)); EQ(1, h(-1)); EQ(0, h(0));
  return 0;
}
