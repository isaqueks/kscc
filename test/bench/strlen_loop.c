// Benchmark: strlen ingênuo em laço (1000 vezes sobre uma string de ~250
// bytes), com strcpy para montar a string.
#include "kstest.h"

static int my_strlen(const char *s) {
  const char *p = s;
  while (*p)
    p++;
  return (int)(p - s);
}

static char *my_strcpy(char *d, const char *s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}

static char buf[256];

int main(void) {
  for (int i = 0; i < 5; i++)
    my_strcpy(buf + i * 50, "The quick brown fox jumps over the lazy dog, KSVM!");
  unsigned total = 0;
  for (int i = 0; i < 1000; i++)
    total += (unsigned)my_strlen(buf + (i & 7));
  ks_put_kv("total", (int)total);
  return 0;
}
