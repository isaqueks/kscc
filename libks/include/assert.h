// <assert.h> da libks (doc 05 §2).
//
// Sem include guard de propósito: C11 7.2p1 manda que cada inclusão
// redefina assert conforme NDEBUG naquele ponto.
//
// Falha: imprime "arquivo:linha: assert(x) falhou", descarrega stdout e
// termina com código 1 (sem atexit). Ver __ks_assert_fail em
// libks/src/assert.c para o porquê de não usar abort().

#undef assert

#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x) ((x) ? (void)0 : __ks_assert_fail(#x, __FILE__, __LINE__))
#endif

#ifndef __KS_ASSERT_FAIL_DECLARED
#define __KS_ASSERT_FAIL_DECLARED
_Noreturn void __ks_assert_fail(const char *expr, const char *file, int line);
#endif
