// test/fuzz/csmith_rt/csmith.h — runtime mínimo do Csmith para o kscc.
//
// Substitui o csmith.h do pacote (que inclui <stdint.h>, <math.h>, usa
// printf e as rotinas de ponto flutuante/64 bits de safe_math.h). Vale
// para os dois lados do diferencial: o kscc e o gcc -m32 compilam o mesmo
// programa com este header, e o checksum é impresso via test/kstest.h.
//
// safe_math_int.h é a parte inteira (até 32 bits) do safe_math.h do pacote
// libcsmith-dev, extraída por test/fuzz/run_csmith.sh (o resto do arquivo
// define funções com float, que o kscc v1 rejeita).
#ifndef KS_CSMITH_H
#define KS_CSMITH_H

#include "kstest.h"

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

#ifndef INT8_MIN
#define INT8_MIN (-128)
#define INT8_MAX 127
#define UINT8_MAX 255
#define INT16_MIN (-32767 - 1)
#define INT16_MAX 32767
#define UINT16_MAX 65535
#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295u
#endif
#ifndef INT_MAX
#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define UINT_MAX 4294967295u
#endif
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef NO_LONGLONG
#error "use csmith --no-longlong --no-math64"
#endif

#define STATIC static
#define UNDEFINED(v) (v)
#define LOG_INDEX
#define LOG_EXEC
#define FUNC_NAME(x) (safe_##x)
#include "safe_math_int.h"
#undef FUNC_NAME

#define INT_BIT (sizeof(int) * CHAR_BIT)
#define _CSMITH_BITFIELD(x) (((x) > INT_BIT) ? ((x) % INT_BIT) : (x))

// O programa só chama printf quando print_hash_value != 0 (nunca, com
// --no-argc); basta existir.
static int ks_csmith_printf(const char *fmt, ...) {
  (void)fmt;
  return 0;
}
#define printf ks_csmith_printf

static uint32_t crc32_tab[256];
static uint32_t crc32_context = 0xFFFFFFFFu;

static void crc32_gentab(void) {
  const uint32_t poly = 0xEDB88320u;
  for (int i = 0; i < 256; i++) {
    uint32_t crc = (uint32_t)i;
    for (int j = 8; j > 0; j--)
      crc = (crc & 1) ? (crc >> 1) ^ poly : crc >> 1;
    crc32_tab[i] = crc;
  }
}

static void crc32_byte(uint8_t b) {
  crc32_context = ((crc32_context >> 8) & 0x00FFFFFF) ^ crc32_tab[(crc32_context ^ b) & 0xFF];
}

static void crc32_8bytes(uint32_t val) {
  crc32_byte((val >> 0) & 0xff);
  crc32_byte((val >> 8) & 0xff);
  crc32_byte((val >> 16) & 0xff);
  crc32_byte((val >> 24) & 0xff);
}

static void transparent_crc(uint32_t val, char *vname, int flag) {
  crc32_8bytes(val);
  if (flag) {
    ks_put_str("...checksum after hashing ");
    ks_put_str(vname);
    ks_put_str(" : ");
    ks_put_hex(crc32_context ^ 0xFFFFFFFFu);
    ks_put_chr('\n');
  }
}

static void transparent_crc_bytes(char *ptr, int nbytes, char *vname, int flag) {
  for (int i = 0; i < nbytes; i++)
    crc32_byte((uint8_t)ptr[i]);
  if (flag) {
    ks_put_str("...checksum after hashing ");
    ks_put_str(vname);
    ks_put_chr('\n');
  }
}

static void platform_main_begin(void) {}

static void platform_main_end(uint32_t crc, int flag) {
  (void)flag;
  ks_put_str("checksum = ");
  ks_put_hex(crc);
  ks_put_chr('\n');
}

#endif
