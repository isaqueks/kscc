// <stdlib.h> da libks (doc 05 §2).
#ifndef __STDLIB_H
#define __STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

// Quantos handlers atexit aceita (C11 exige pelo menos 32; a libks fica
// com 8 para caber em MCUs com pouca RAM, doc 05 §2).
#define KS_ATEXIT_MAX 8

typedef struct {
  int quot;
  int rem;
} div_t;

typedef struct {
  long quot;
  long rem;
} ldiv_t;

int abs(int n);
long labs(long n);
div_t div(int num, int den);
ldiv_t ldiv(long num, long den);

int atoi(const char *s);
long atol(const char *s);
long strtol(const char *restrict s, char **restrict end, int base);
unsigned long strtoul(const char *restrict s, char **restrict end, int base);

int rand(void);
void srand(unsigned seed);

_Noreturn void exit(int code);
_Noreturn void _Exit(int code);
_Noreturn void abort(void);
int atexit(void (*fn)(void));

void qsort(void *base, size_t n, size_t size,
           int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t n, size_t size,
              int (*cmp)(const void *, const void *));

void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t size);
void free(void *p);

// Extensão da libks: percorre o heap e confere os cabeçalhos dos blocos.
// Devolve 0 se está consistente, -1 se achou corrupção.
int ks_heap_check(void);

#endif
