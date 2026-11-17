#include "test.h"

#pragma once

#include "test/chibicc/pragma-once.c" // kscc: caminho da suite no kscc

int main() {
  kscc_ok(); // kscc: printf só na fase 4 (libks)
  return 0;
}
