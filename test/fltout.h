// test/fltout.h — saída de floats para os testes diferenciais (test/diff) e
// o fuzz (test/fuzz/gen_expr.py), igual no kscc e no gcc -m32 -msse
// -mfpmath=sse.
//
//   fl_bits(f)       os bits do binary32 (unsigned)
//   fl_put(name, f)  "name = xxxxxxxx\n" (%08x), ou "name = nan\n"
//   fl_hex(f)        só os 8 dígitos (ou "nan") seguidos de espaço
//
// Todo NaN sai como "nan": a KSVM produz sempre o NaN canônico 0x7FC00000
// (doc 04 §5.7 da ksvm2), o x86 produz 0xFFC00000 e propaga payloads.
// Ambos são NaN; só o padrão difere. Os demais valores (inclusive -0, inf
// e subnormais) saem bit a bit.
#ifndef FLTOUT_H
#define FLTOUT_H

#include <stdio.h>

static inline unsigned fl_bits(float f) {
  union {
    float f;
    unsigned u;
  } b;
  b.f = f;
  return b.u;
}

static inline float fl_from(unsigned u) {
  union {
    float f;
    unsigned u;
  } b;
  b.u = u;
  return b.f;
}

static inline void fl_hex(float f) {
  if (f != f)
    printf("nan      ");
  else
    printf("%08x ", fl_bits(f));
}

static inline void fl_put(const char *name, float f) {
  printf("%s = ", name);
  fl_hex(f);
  printf("\n");
}

#endif
