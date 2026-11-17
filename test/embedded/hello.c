// test/embedded/hello.c — "KSVM C on <plataforma>" e halt (código 0).
// A plataforma vem de -DKS_PLATFORM="\"AVR\"" (test/embedded/run.sh).
#include "kstest.h"

#ifndef KS_PLATFORM
#define KS_PLATFORM "desktop"
#endif

int main(void) {
  ks_put_str("KSVM C on " KS_PLATFORM "\n");
  return 0;
}
