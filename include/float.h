// <float.h> do kscc (C11 5.2.4.2.2).
//
// kscc: float, double e long double são todos IEEE 754 binary32 (a KSVM
// só tem a extensão F de 32 bits; doc 02 §1), então os valores DBL_* e
// LDBL_* são iguais aos FLT_*. É uma divergência deliberada da norma, que
// pede para double pelo menos 10 dígitos decimais (DBL_DIG >= 10); o
// avr-gcc antigo com -fshort-double fazia o mesmo.
#ifndef __STDFLOAT_H
#define __STDFLOAT_H

#define FLT_EVAL_METHOD 0 // C11 5.2.4.2.2p9: cada operação no próprio tipo
#define FLT_RADIX 2
#define FLT_ROUNDS 1      // C11 5.2.4.2.2p8: para o mais próximo (RNE)
#define DECIMAL_DIG 9     // dígitos que distinguem qualquer binary32

#define FLT_HAS_SUBNORM 1
#define FLT_DECIMAL_DIG 9
#define FLT_DIG 6
#define FLT_EPSILON 0x1p-23F
#define FLT_MANT_DIG 24
#define FLT_MAX 0x1.fffffep+127F
#define FLT_MAX_10_EXP 38
#define FLT_MAX_EXP 128
#define FLT_MIN 0x1p-126F
#define FLT_MIN_10_EXP -37
#define FLT_MIN_EXP -125
#define FLT_TRUE_MIN 0x1p-149F

#define DBL_HAS_SUBNORM 1
#define DBL_DECIMAL_DIG 9
#define DBL_DIG 6
#define DBL_EPSILON 0x1p-23
#define DBL_MANT_DIG 24
#define DBL_MAX 0x1.fffffep+127
#define DBL_MAX_10_EXP 38
#define DBL_MAX_EXP 128
#define DBL_MIN 0x1p-126
#define DBL_MIN_10_EXP -37
#define DBL_MIN_EXP -125
#define DBL_TRUE_MIN 0x1p-149

#define LDBL_HAS_SUBNORM 1
#define LDBL_DECIMAL_DIG 9
#define LDBL_DIG 6
#define LDBL_EPSILON 0x1p-23L
#define LDBL_MANT_DIG 24
#define LDBL_MAX 0x1.fffffep+127L
#define LDBL_MAX_10_EXP 38
#define LDBL_MAX_EXP 128
#define LDBL_MIN 0x1p-126L
#define LDBL_MIN_10_EXP -37
#define LDBL_MIN_EXP -125
#define LDBL_TRUE_MIN 0x1p-149L

#endif
