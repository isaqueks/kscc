// Global const fica em .rodata (região de código, só leitura): ler
// funciona; escrever (por um ponteiro sem const) dá FAULT_MEM_PERM.
// expect-exit: 75
#include "minirt.h"
const int limit = 10;
const int table[3] = {1, 2, 3};
int mutable_g = 5;

int main() {
  EQ(10, limit);
  EQ(6, table[0] + table[1] + table[2]);
  mutable_g = 6; // .data continua gravável
  EQ(6, mutable_g);
  int *p = (int *)&table[1];
  *p = 99;
  return 0;
}
