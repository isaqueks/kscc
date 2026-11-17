// Com NDEBUG, assert não avalia a expressão; incluir <assert.h> de novo
// sem NDEBUG reativa (C11 7.2p1: assert.h não tem include guard).
#define NDEBUG
#include <assert.h>
#include "minirt.h"
// expect-out: ok

static int calls;
static int touch(void) { return ++calls; }

int main() {
  assert(touch() == 100);
  assert(0);
  EQ(0, calls);

#undef NDEBUG
#include <assert.h>
  assert(touch() == 1);
  EQ(1, calls);
  ks_put_str("ok\n");
  return 0;
}
