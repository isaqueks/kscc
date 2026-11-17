#include "kstest.h"
// &a com a array é ponteiro para o array (C11 6.5.3.2p3): &a + 1 avança
// sizeof a. O kscc (como o chibicc) dava ao &a o tipo do elemento.
int arr[10];
int m[3][4];
int *end = (int *)(&arr + 1);
char *mend = (char *)(&m[1] + 1);
int main(void) {
  int loc[5];
  int (*pa)[10] = &arr;
  ks_put_kv("&arr+1", (int)((char *)(&arr + 1) - (char *)arr));
  ks_put_kv("sizeof *&arr", (int)sizeof(*&arr));
  ks_put_kv("sizeof &arr[0]", (int)sizeof(&arr[0]));
  ks_put_kv("end", (int)(end - arr));
  ks_put_kv("mend", (int)(mend - (char *)m));
  ks_put_kv("&loc+1", (int)((char *)(&loc + 1) - (char *)loc));
  ks_put_kv("deref", (int)(&(*(&arr))[7] - arr));
  ks_put_kv("pa", (int)((char *)(pa + 1) - (char *)&arr));
  return 0;
}
