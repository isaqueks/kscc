// Definição provisória de array sem tamanho que fica sem definição na
// unidade (C11 6.9.2p2): vale `T a[1]`, ou o tamanho de outra declaração
// do mesmo nome. Antes o objeto tinha 0 bytes e escrever a[0] corrompia o
// objeto seguinte em silêncio.
#include "minirt.h"
int arr[];
int after = 5;
static int sarr[];
int sa2 = 6;
int big[4];
int big[];
int after2 = 7;
extern int ext[3];
int ext[];
int after3 = 8;
int main() {
  arr[0] = 77;
  sarr[0] = 88;
  for (int i = 0; i < 4; i++)
    big[i] = 100 + i;
  for (int i = 0; i < 3; i++)
    ext[i] = 200 + i;
  EQ(77, arr[0]);
  EQ(5, after);
  EQ(88, sarr[0]);
  EQ(6, sa2);
  EQ(103, big[3]);
  EQ(7, after2);
  EQ(202, ext[2]);
  EQ(8, after3);
  return 0;
}
