// kscc: <stdint.h> ILP32 (doc 02 §1, doc 05 §2). Novo no kscc.
//
// Os tipos de 64 bits (int64_t, uint64_t, INT64_MAX...) só chegam com
// `long long` na fase 5; até lá intmax_t é o maior inteiro suportado,
// de 32 bits.
#ifndef __STDINT_H
#define __STDINT_H

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;

// Os "fast" são de 32 bits: a VM só tem registradores de 32 bits, e
// carregar/gravar 8 ou 16 bits custa o mesmo, mas a aritmética não.
typedef int8_t int_fast8_t;
typedef int32_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint8_t uint_fast8_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;

typedef int intptr_t;
typedef unsigned int uintptr_t;
typedef long intmax_t;
typedef unsigned long uintmax_t;

#define INT8_MIN (-128)
#define INT16_MIN (-32767 - 1)
#define INT32_MIN (-2147483647 - 1)
#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST8_MAX INT8_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define UINT_LEAST32_MAX UINT32_MAX

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST16_MIN INT32_MIN
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST8_MAX INT8_MAX
#define INT_FAST16_MAX INT32_MAX
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST8_MAX UINT8_MAX
#define UINT_FAST16_MAX UINT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define INTMAX_MIN (-2147483647L - 1)
#define INTMAX_MAX 2147483647L
#define UINTMAX_MAX 4294967295UL

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#define SIZE_MAX UINT32_MAX
#define WCHAR_MIN 0
#define WCHAR_MAX UINT32_MAX
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX

#define INT8_C(x) (x)
#define INT16_C(x) (x)
#define INT32_C(x) (x)
#define UINT8_C(x) (x)
#define UINT16_C(x) (x)
#define UINT32_C(x) (x ## U)
#define INTMAX_C(x) (x ## L)
#define UINTMAX_C(x) (x ## UL)

#endif
