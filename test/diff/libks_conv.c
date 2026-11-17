// strtol/strtoul/atoi, <ctype.h>, <string.h> e qsort/bsearch da libks x
// glibc. Sob o kscc os headers são os da libks; sob o gcc, os da glibc.
// Só o que o C11 define: memcmp/strcmp/strncmp pelo sinal, ctype de EOF
// a 255 (locale "C"), qsort com chaves distintas.
#define _POSIX_C_SOURCE 200809L  // strnlen na glibc
#include <ctype.h>
#include <stdlib.h>
#ifdef __kscc__
#include <errno.h>
#else
// O <errno.h> da glibc em -m32 precisa de asm/errno.h, que falta no
// ambiente de build (linux-libc-dev:i386); o errno da glibc é este.
extern int *__errno_location(void);
#define errno (*__errno_location())
#define ERANGE 34
#endif
#include <string.h>
#include "kstest.h"

static const char *inputs[] = {
    "0", "1", "-1", "+1", "  42", "\t\n\v\f\r -42x", "2147483647", "2147483648",
    "-2147483648", "-2147483649", "4294967295", "4294967296", "-4294967295",
    "-4294967296", "99999999999999999999", "0x", "0x1g", "0X7fffffff",
    "0x80000000", "0xffffffff", "-0x80000000", "0777", "08", "0b101", "z",
    "Zz", "zzzzzzz", "  +", "-", "", "  -0", "1e5", "0x0x1", "00x1", "+-1",
    "12abc", "abc", "1010102", "7fffffff", "80000000"};

static const int bases[] = {0, 2, 8, 10, 16, 36};

static int sign(int v) { return (v > 0) - (v < 0); }

static int cmp_int(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return (x > y) - (x < y);
}

static int cmp_str(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}

int main(void) {
  int n = sizeof(inputs) / sizeof(inputs[0]);

  for (int i = 0; i < n; i++)
    for (int b = 0; b < 6; b++) {
      char *end;
      errno = 0;
      long v = strtol(inputs[i], &end, bases[b]);
      int e1 = errno;
      int used = end - inputs[i];
      errno = 0;
      unsigned long u = strtoul(inputs[i], &end, bases[b]);
      int e2 = errno;
      ks_put_str("\"");
      ks_put_str(inputs[i]);
      ks_put_str("\" base ");
      ks_put_int(bases[b]);
      ks_put_str(": strtol ");
      ks_put_int((int)v);
      ks_put_str(" end ");
      ks_put_int(used);
      ks_put_str(e1 == ERANGE ? " ERANGE" : e1 ? " erro" : "");
      ks_put_str(" | strtoul ");
      ks_put_uint((unsigned)u);
      ks_put_str(" end ");
      ks_put_int((int)(end - inputs[i]));
      ks_put_str(e2 == ERANGE ? " ERANGE" : e2 ? " erro" : "");
      ks_put_chr('\n');
    }
  for (int i = 0; i < n; i++)
    if (strchr(inputs[i], 'e') == 0 && strchr(inputs[i], '9') == 0)  // sem estouro
      ks_put_kv(inputs[i], atoi(inputs[i]));

  // ctype de EOF a 255
  for (int c = -1; c < 256; c++) {
    int bits = (!!isdigit(c)) | (!!isalpha(c)) << 1 | (!!isalnum(c)) << 2 |
               (!!isspace(c)) << 3 | (!!isupper(c)) << 4 | (!!islower(c)) << 5 |
               (!!isxdigit(c)) << 6 | (!!isprint(c)) << 7;
    if (bits || toupper(c) != c || tolower(c) != c) {
      ks_put_int(c);
      ks_put_chr(' ');
      ks_put_int(bits);
      ks_put_chr(' ');
      ks_put_int(toupper(c));
      ks_put_chr(' ');
      ks_put_int(tolower(c));
      ks_put_chr('\n');
    }
  }

  // <string.h>
  const char *hay = "a banana bandana; ab\xe9 abc";
  const char *needles[] = {"", "a", "an", "ana", "band", "bc", "\xe9", "x", "a banana bandana; ab\xe9 abcd"};
  for (int i = 0; i < 9; i++) {
    const char *r = strstr(hay, needles[i]);
    ks_put_int(r ? (int)(r - hay) : -1);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  const int chars[] = {'a', 'n', ';', 0, 0xe9, -23, 'z', 'a' + 256};
  for (int i = 0; i < 8; i++) {
    const char *r1 = strchr(hay, chars[i]), *r2 = strrchr(hay, chars[i]);
    const char *r3 = memchr(hay, chars[i], strlen(hay) + 1);
    ks_put_int(r1 ? (int)(r1 - hay) : -1);
    ks_put_chr('/');
    ks_put_int(r2 ? (int)(r2 - hay) : -1);
    ks_put_chr('/');
    ks_put_int(r3 ? (int)(r3 - hay) : -1);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  const char *pairs[][2] = {{"", ""}, {"a", ""}, {"", "a"}, {"abc", "abd"}, {"abd", "abc"},
                            {"ab", "abc"}, {"\xe9", "a"}, {"a\xe9", "a\x01"}, {"same", "same"}};
  for (int i = 0; i < 9; i++) {
    ks_put_int(sign(strcmp(pairs[i][0], pairs[i][1])));
    ks_put_int(sign(strncmp(pairs[i][0], pairs[i][1], 2)));
    ks_put_int(sign(memcmp(pairs[i][0], pairs[i][1], 1)));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  for (int k = 0; k <= 8; k++) {
    char buf[12];
    memset(buf, '#', sizeof(buf));
    strncpy(buf, "abcde", k);
    buf[11] = 0;
    for (int j = 0; j < 11; j++)
      ks_put_chr(buf[j] ? buf[j] : '0');
    memcpy(buf, "xy", 3);
    strncat(buf, "12345", k);
    ks_put_chr(' ');
    ks_put_str(buf);
    ks_put_chr(' ');
    ks_put_int((int)strnlen("abcde", k));
    ks_put_chr('\n');
  }

  // qsort/bsearch com chaves distintas
  int v[57];
  for (int i = 0; i < 57; i++)
    v[i] = (i * 7919 + 13) % 1009 - 500;
  qsort(v, 57, sizeof(int), cmp_int);
  for (int i = 0; i < 57; i++) {
    ks_put_int(v[i]);
    ks_put_chr(i % 19 == 18 ? '\n' : ' ');
  }
  int found = 0, missed = 0;
  for (int key = -510; key <= 510; key++) {
    int *r = bsearch(&key, v, 57, sizeof(int), cmp_int);
    if (r && *r == key)
      found += r - v;
    else if (!r)
      missed++;
  }
  ks_put_kv("bsearch achou (soma dos indices)", found);
  ks_put_kv("bsearch nao achou", missed);
  const char *words[] = {"pera", "uva", "abacate", "kiwi", "banana", "abacaxi", "Uva", "", "b"};
  qsort(words, 9, sizeof(char *), cmp_str);
  for (int i = 0; i < 9; i++) {
    ks_put_str(words[i]);
    ks_put_chr(',');
  }
  ks_put_chr('\n');

  div_t d = div(-7, 2);
  ldiv_t ld = ldiv(7L, -2L);
  ks_put_kv("div", d.quot * 100 + d.rem);
  ks_put_kv("ldiv", (int)(ld.quot * 100 + ld.rem));
  ks_put_kv("abs", abs(-5) + labs(-6L));
  return 0;
}
