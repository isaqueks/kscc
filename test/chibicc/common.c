// kscc: substitui test/chibicc/common (que usava printf/stdio/stdlib da
// glibc). Até a libks (fase 4), assert() imprime com ks_sys_puts e converte
// inteiros para decimal em C puro; printf fica de fora (os testes chamam
// kscc_ok() no lugar de printf("OK\n")). As funções auxiliares que dependem
// de struct por valor, varargs ou ponto flutuante ficam atrás de
// KSCC_PHASE2 / KSCC_PHASE3 / KSCC_HAS_FLOAT.

int ks_sys_puts(const char *p, int len);
void ks_sys_exit(int code);

static int str_len(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  return n;
}

static void put_str(const char *s) {
  ks_sys_puts(s, str_len(s));
}

static void put_int(int v) {
  char buf[12];
  int i = 12;
  unsigned u = v < 0 ? -(unsigned)v : (unsigned)v;
  do {
    buf[--i] = '0' + u % 10;
    u /= 10;
  } while (u);
  if (v < 0)
    buf[--i] = '-';
  ks_sys_puts(buf + i, 12 - i);
}

void assert(int expected, int actual, char *code) {
  put_str(code);
  put_str(" => ");
  if (expected == actual) {
    put_int(actual);
    put_str("\n");
  } else {
    put_int(expected);
    put_str(" expected but got ");
    put_int(actual);
    put_str("\n");
    ks_sys_exit(1);
  }
}

void kscc_ok(void) {
  put_str("OK\n");
}

void exit(int n) {
  ks_sys_exit(n);
}

// Subconjunto de <string.h> usado pelos testes (a libks real é a fase 4).
int strcmp(char *p, char *q) {
  while (*p && *p == *q)
    p++, q++;
  return (unsigned char)*p - (unsigned char)*q;
}

int strncmp(char *p, char *q, long n) {
  for (; n > 0; n--, p++, q++) {
    if (*p != *q)
      return (unsigned char)*p - (unsigned char)*q;
    if (!*p)
      return 0;
  }
  return 0;
}

int memcmp(char *p, char *q, long n) {
  for (; n > 0; n--, p++, q++)
    if (*p != *q)
      return (unsigned char)*p - (unsigned char)*q;
  return 0;
}

long strlen(char *s) {
  return str_len(s);
}

void *memcpy(void *dest, void *src, long n) {
  char *d = dest, *s = src;
  while (n-- > 0)
    *d++ = *s++;
  return dest;
}

void *memset(void *s, int c, long n) {
  char *p = s;
  while (n-- > 0)
    *p++ = c;
  return s;
}

static int static_fn() { return 5; }
int ext1 = 5;
int *ext2 = &ext1;
int ext3 = 7;
int ext_fn1(int x) { return x; }
int ext_fn2(int x) { return x; }
int common_ext2 = 3;
static int common_local;

int false_fn() { return 512; }
int true_fn() { return 513; }
int char_fn() { return (2<<8)+3; }
int short_fn() { return (2<<16)+5; }

int uchar_fn() { return (2<<10)-1-4; }
int ushort_fn() { return (2<<20)-1-7; }

int schar_fn() { return (2<<10)-1-4; }
int sshort_fn() { return (2<<20)-1-7; }

#include <stdarg.h>

// kscc: convenção variádica da KSVM (tudo na pilha, va_list = char*).
int add_all(int n, ...) {
  va_list ap;
  va_start(ap, n);

  int sum = 0;
  for (int i = 0; i < n; i++)
    sum += va_arg(ap, int);
  return sum;
}

int add10_int(int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9, int x10) {
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}

#ifdef KSCC_HAS_FLOAT
float add_float(float x, float y) {
  return x + y;
}

double add_double(double x, double y) {
  return x + y;
}

float add10_float(float x1, float x2, float x3, float x4, float x5, float x6, float x7, float x8, float x9, float x10) {
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}

double add10_double(double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8, double x9, double x10) {
  return x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10;
}
#endif

#ifdef KSCC_PHASE2
typedef struct { int a,b; short c; char d; } Ty4;
typedef struct { unsigned char a[3]; } Ty6;
typedef struct { long a, b, c; } Ty7;

int struct_test4(Ty4 x, int n) {
  switch (n) {
  case 0: return x.a;
  case 1: return x.b;
  case 2: return x.c;
  default: return x.d;
  }
}

int struct_test6(Ty6 x, int n) {
  return x.a[n];
}

int struct_test7(Ty7 x, int n) {
  switch (n) {
  case 0: return x.a;
  case 1: return x.b;
  default: return x.c;
  }
}

Ty4 struct_test24(void) {
  return (Ty4){10, 20, 30, 40};
}

Ty6 struct_test26(void) {
  return (Ty6){10, 20, 30};
}

typedef struct { unsigned char a[10]; } Ty20;
typedef struct { unsigned char a[20]; } Ty21;

Ty20 struct_test27(void) {
  return (Ty20){10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
}

Ty21 struct_test28(void) {
  return (Ty21){1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
}
#endif

#if defined(KSCC_PHASE2) && defined(KSCC_HAS_FLOAT)
typedef struct { int a; float b; double c; } Ty5;

int struct_test5(Ty5 x, int n) {
  switch (n) {
  case 0: return x.a;
  case 1: return x.b;
  default: return x.c;
  }
}

Ty5 struct_test25(void) {
  return (Ty5){10, 20, 30};
}
#endif
