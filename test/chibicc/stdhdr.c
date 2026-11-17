#include "test.h"
#include <float.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

int main() {
  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
