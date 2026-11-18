// <stdio.h> da libks (doc 05 §2).
//
// Não há FILE genérico nem fopen: stdin, stdout e stderr são constantes
// que só servem para as funções f* abaixo. A KSVM tem um único console
// (syscalls PUTC/PUTS), então stdout e stderr saem no mesmo lugar; a
// diferença é que stdout tem buffer de 128 bytes, descarregado em '\n',
// fflush, exit e antes de ler stdin, e stderr escreve na hora (depois de
// descarregar stdout, para manter a ordem).
//
// printf e família: %d %i %u %x %X %o %c %s %p %n %%, flags - 0 + espaço
// e #, largura e precisão (inclusive *), modificadores hh h l z t j (todos
// de 32 bits). %f %F %e %E %g %G %a %A (também com L): conversão exata do
// binary32, arredondada como no glibc (doc 05 §2). %lld imprime "<?>" até
// a fase 5.
#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define BUFSIZ 128

typedef struct __ks_file FILE;  // incompleto: só os três abaixo existem

#define stdin ((FILE *)1)
#define stdout ((FILE *)2)
#define stderr ((FILE *)3)

int putchar(int c);
int getchar(void);
int puts(const char *s);
char *gets_s(char *buf, size_t n);  // sem o '\n'; NULL no fim da entrada

int fputc(int c, FILE *f);
int putc(int c, FILE *f);
int fgetc(FILE *f);
int getc(FILE *f);
int fputs(const char *restrict s, FILE *restrict f);
char *fgets(char *restrict buf, int n, FILE *restrict f);
int fflush(FILE *f);  // NULL ou stdout: descarrega stdout

int printf(const char *restrict fmt, ...);
int fprintf(FILE *restrict f, const char *restrict fmt, ...);
int sprintf(char *restrict buf, const char *restrict fmt, ...);
int snprintf(char *restrict buf, size_t n, const char *restrict fmt, ...);
int vprintf(const char *restrict fmt, va_list ap);
int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);
int vsprintf(char *restrict buf, const char *restrict fmt, va_list ap);
int vsnprintf(char *restrict buf, size_t n, const char *restrict fmt, va_list ap);

#endif
