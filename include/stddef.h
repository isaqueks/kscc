// kscc: ILP32 (doc 02 §1). Derivado de chibicc/include/stddef.h.
#ifndef __STDDEF_H
#define __STDDEF_H

#define NULL ((void *)0)

typedef unsigned int size_t;   // kscc: ILP32 (era unsigned long, LP64)
typedef int ptrdiff_t;         // kscc: ILP32 (era long, LP64)
typedef unsigned int wchar_t;
typedef int max_align_t;       // kscc: alinhamento máximo natural é 4

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif
