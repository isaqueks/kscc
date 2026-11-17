// kscc: <limits.h> ILP32 (doc 02 §1). Novo no kscc.
//
// char é signed (como no x86). LLONG_* só na fase 5.
#ifndef __LIMITS_H
#define __LIMITS_H

#define CHAR_BIT 8
#define MB_LEN_MAX 4

#define SCHAR_MIN (-128)
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX

#define SHRT_MIN (-32767 - 1)
#define SHRT_MAX 32767
#define USHRT_MAX 65535

#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define UINT_MAX 4294967295U

#define LONG_MIN (-2147483647L - 1)
#define LONG_MAX 2147483647L
#define ULONG_MAX 4294967295UL

#endif
