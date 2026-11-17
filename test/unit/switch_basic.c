// switch com default no meio, fallthrough e break.
#include "minirt.h"
int classify(int x) {
  int r = 0;
  switch (x) {
  case 1:
    r = 10;
    break;
  default:
    r = -1;
    break;
  case 2:
  case 3:
    r = 23;
    break;
  case 4:
    r += 4;
  case 5:
    r += 5;
  case 6:
    r += 6;
    break;
  case -7:
    r = 77;
  }
  return r;
}
int main() {
  EQ(10, classify(1));
  EQ(23, classify(2));
  EQ(23, classify(3));
  EQ(15, classify(4));
  EQ(11, classify(5));
  EQ(6, classify(6));
  EQ(77, classify(-7));
  EQ(-1, classify(100));
  int n = 0;
  switch (n) { case 1: n = 5; }
  EQ(0, n);
  switch (n) { }
  return 0;
}
