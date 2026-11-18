// Tipos: float, double e long double têm 4 bytes, alinhamento 4 e a mesma
// representação binary32 (doc 02 §1), mas continuam tipos distintos para
// _Generic e compatibilidade. Macros predefinidas e <float.h>.
#include <float.h>
#include "float_test.h"

#define KIND(x) _Generic((x), float: 1, double: 2, long double: 3, int: 4, default: 0)

struct S { char c; double d; };
union U { float f; double d; long double l; unsigned u; };

int main() {
  EQ(4, (int)sizeof(float)); EQ(4, (int)sizeof(double)); EQ(4, (int)sizeof(long double));
  EQ(4, (int)_Alignof(float)); EQ(4, (int)_Alignof(double)); EQ(4, (int)_Alignof(long double));
  EQ(8, (int)sizeof(struct S));
  EQ(4, (int)sizeof(union U));
  EQ(4, __SIZEOF_FLOAT__); EQ(4, __SIZEOF_DOUBLE__); EQ(4, __SIZEOF_LONG_DOUBLE__);
  EQ(0, __FLT_EVAL_METHOD__); EQ(1, __kscc_float__);
#ifdef __STDC_IEC_559__
  EQ(0, 1);  // double não é binary64: não definida
#endif

  // _Generic vê o tipo da linguagem
  EQ(1, KIND(1.0f)); EQ(2, KIND(1.0)); EQ(3, KIND(1.0L)); EQ(4, KIND(1));
  EQ(2, KIND(1.0f + 1.0)); EQ(3, KIND(1.0 + 1.0L)); EQ(1, KIND(1.0f * 2));
  float f = 0; double d = 0;
  EQ(1, KIND(f)); EQ(2, KIND(d)); EQ(1, KIND(-f)); EQ(2, KIND(f ? d : f));
  EQ(1, __builtin_types_compatible_p(float, float));
  EQ(0, __builtin_types_compatible_p(float, double));
  EQ(0, __builtin_types_compatible_p(double, long double));
  typeof(1.5) t = 0;
  EQ(2, KIND(t));

  // a mesma representação: um double guarda exatamente os bits do float
  union U u;
  u.d = 0.1;
  EQ((int)0x3DCCCCCD, (int)u.u);
  u.l = 0.1L;
  EQB(0x3DCCCCCD, u.f);
  EQ(1, 0.1 == 0.1f && 0.1L == 0.1f);

  // <float.h>: DBL_* e LDBL_* iguais aos FLT_*
  EQB(0x34000000, FLT_EPSILON); EQB(0x34000000, DBL_EPSILON); EQB(0x34000000, LDBL_EPSILON);
  EQB(0x7F7FFFFF, FLT_MAX); EQB(0x7F7FFFFF, DBL_MAX); EQB(0x7F7FFFFF, LDBL_MAX);
  EQB(0x00800000, FLT_MIN); EQB(0x00800000, DBL_MIN);
  EQB(0x00000001, FLT_TRUE_MIN); EQB(0x00000001, DBL_TRUE_MIN);
  EQ(24, FLT_MANT_DIG); EQ(24, DBL_MANT_DIG); EQ(24, LDBL_MANT_DIG);
  EQ(6, FLT_DIG); EQ(6, DBL_DIG); EQ(9, DECIMAL_DIG); EQ(9, DBL_DECIMAL_DIG);
  EQ(128, DBL_MAX_EXP); EQ(-125, DBL_MIN_EXP); EQ(38, DBL_MAX_10_EXP); EQ(-37, DBL_MIN_10_EXP);
  EQ(0, FLT_EVAL_METHOD); EQ(1, FLT_ROUNDS); EQ(2, FLT_RADIX);
  EQ(1, FLT_MAX + FLT_MAX == rt(1) / 0);
  EQ(1, 1.0f + FLT_EPSILON != 1.0f);
  EQ(1, 1.0f + FLT_EPSILON / 2 == 1.0f);
  return 0;
}
