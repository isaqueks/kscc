// Todos os headers da libks e do compilador juntos, duas vezes (guards),
// com os tamanhos e limites ILP32 de stdint.h/limits.h e as macros de
// iso646.h, stdbool.h, stdalign.h e stddef.h.
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <iso646.h>
#include <ksvm/gpio.h>
#include <ksvm/syscall.h>
#include <ksvm/time.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <ksvm/syscall.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minirt.h"

typedef struct {
  char c;
  int i;
} S;

int main() {
  EQ(1, sizeof(int8_t));
  EQ(2, sizeof(int16_t));
  EQ(4, sizeof(int32_t));
  EQ(4, sizeof(intptr_t));
  EQ(4, sizeof(uintptr_t));
  EQ(4, sizeof(intmax_t));
  EQ(4, sizeof(size_t));
  EQ(4, sizeof(ptrdiff_t));
  EQ(4, sizeof(int_fast16_t));
  EQ(-128, INT8_MIN);
  EQ(32767, INT16_MAX);
  EQ(1, INT32_MIN < 0 && INT32_MAX > 0);
  EQ(1, UINT32_MAX == 4294967295u);
  EQ(1, SIZE_MAX == 0xffffffffu);
  EQ(1, UINT32_C(1) == 1u);

  EQ(8, CHAR_BIT);
  EQ(-128, CHAR_MIN);  // char tem sinal
  EQ(127, CHAR_MAX);
  EQ(255, UCHAR_MAX);
  EQ(-32768, SHRT_MIN);
  EQ(1, INT_MAX == 2147483647 && INT_MIN == -INT_MAX - 1);
  EQ(1, LONG_MAX == INT_MAX && ULONG_MAX == UINT_MAX);

  EQ(1, 1 and 1);
  EQ(1, 0 or 1);
  EQ(1, not 0);
  EQ(7, 3 bitor 5 bitand 6);
  EQ(1, true);
  EQ(4, alignof(S));
  EQ(4, offsetof(S, i));
  EQ(0, (int)NULL);
  EQ(-1, EOF);
  EQ(1, EXIT_FAILURE);
  EQ(32767, RAND_MAX);
  EQ(1, ENOSYS);
  EQ(5, EAGAIN);
  EQ(1, EDOM != EILSEQ && EILSEQ != ERANGE && EDOM != ERANGE);  // C11 7.5p2
  EQ(1, stdin != stdout && stdout != stderr && stdin != 0);
  return 0;
}
