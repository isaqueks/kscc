// <string.h> da libks (doc 05 §2).
//
// O codegen do kscc copia structs inline; estas funções só são chamadas
// pelo usuário e pelo resto da libks. memcpy/memset trabalham com palavras
// de 4 bytes quando os endereços permitem: a VM executa uma instrução por
// vez, então 4 bytes por iteração dão ~4x menos instruções.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int aligned4(const void *a, const void *b) {
  return (((uintptr_t)a | (uintptr_t)b) & 3) == 0;
}

void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;

  if (aligned4(d, s)) {
    for (; n >= 4; n -= 4, d += 4, s += 4)
      *(unsigned *)d = *(const unsigned *)s;
  }
  while (n--)
    *d++ = *s++;
  return dst;
}

// Cópia para trás quando o destino começa dentro da origem; nos demais
// casos a cópia para frente de memcpy já é segura.
void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;

  if (d <= s || d >= s + n)
    return memcpy(dst, src, n);

  d += n;
  s += n;
  while (n--)
    *--d = *--s;
  return dst;
}

void *memset(void *p, int c, size_t n) {
  unsigned char *d = p;
  unsigned char b = c;

  if (aligned4(d, d)) {
    unsigned word = b * 0x01010101u;
    for (; n >= 4; n -= 4, d += 4)
      *(unsigned *)d = word;
  }
  while (n--)
    *d++ = b;
  return p;
}

int memcmp(const void *a, const void *b, size_t n) {
  const unsigned char *p = a, *q = b;

  for (; n > 0; n--, p++, q++)
    if (*p != *q)
      return *p - *q;
  return 0;
}

void *memchr(const void *s, int c, size_t n) {
  const unsigned char *p = s;

  for (; n > 0; n--, p++)
    if (*p == (unsigned char)c)
      return (void *)p;
  return NULL;
}

size_t strlen(const char *s) {
  const char *p = s;

  while (*p)
    p++;
  return p - s;
}

size_t strnlen(const char *s, size_t max) {
  size_t n = 0;

  while (n < max && s[n])
    n++;
  return n;
}

char *strcpy(char *restrict dst, const char *restrict src) {
  char *d = dst;

  while ((*d++ = *src++))
    ;
  return dst;
}

// Como o C: copia até n bytes e completa com '\0' até n; sem '\0' final se
// src tem n caracteres ou mais.
char *strncpy(char *restrict dst, const char *restrict src, size_t n) {
  size_t i = 0;

  for (; i < n && src[i]; i++)
    dst[i] = src[i];
  for (; i < n; i++)
    dst[i] = '\0';
  return dst;
}

char *strcat(char *restrict dst, const char *restrict src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

// Acrescenta no máximo n caracteres de src e sempre termina com '\0'.
char *strncat(char *restrict dst, const char *restrict src, size_t n) {
  char *d = dst + strlen(dst);

  for (; n > 0 && *src; n--)
    *d++ = *src++;
  *d = '\0';
  return dst;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b)
    a++, b++;
  return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
  for (; n > 0; n--, a++, b++) {
    if (*a != *b)
      return (unsigned char)*a - (unsigned char)*b;
    if (!*a)
      return 0;
  }
  return 0;
}

// O '\0' final faz parte da string: strchr(s, 0) aponta para ele.
char *strchr(const char *s, int c) {
  for (;; s++) {
    if (*s == (char)c)
      return (char *)s;
    if (!*s)
      return NULL;
  }
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;

  for (;; s++) {
    if (*s == (char)c)
      last = s;
    if (!*s)
      return (char *)last;
  }
}

// Busca ingênua O(n·m): as strings de programas embarcados são curtas, e
// algoritmos melhores (KMP, two-way) custam tabela ou código.
char *strstr(const char *haystack, const char *needle) {
  size_t n = strlen(needle);

  if (n == 0)
    return (char *)haystack;
  for (; *haystack; haystack++)
    if (*haystack == *needle && !strncmp(haystack, needle, n))
      return (char *)haystack;
  return NULL;
}

char *strdup(const char *s) {
  size_t n = strlen(s) + 1;
  char *p = malloc(n);

  if (p)
    memcpy(p, s, n);
  return p;
}
