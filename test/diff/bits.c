// Bit twiddling: popcount, paridade, gray code, rotações, inversão de bits,
// clz/ctz manuais, menor bit, potência de 2, intercalação de bits (Morton).
#include "kstest.h"

static int popcount(unsigned x) {
  x = x - ((x >> 1) & 0x55555555u);
  x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
  x = (x + (x >> 4)) & 0x0F0F0F0Fu;
  return (int)((x * 0x01010101u) >> 24);
}

static int popcount_slow(unsigned x) {
  int n = 0;
  for (; x; x &= x - 1)
    n++;
  return n;
}

static unsigned rotl(unsigned x, int r) { return (x << (r & 31)) | (x >> ((32 - r) & 31)); }
static unsigned rotr(unsigned x, int r) { return (x >> (r & 31)) | (x << ((32 - r) & 31)); }
static unsigned gray(unsigned x) { return x ^ (x >> 1); }
static unsigned gray_inv(unsigned g) {
  for (unsigned s = 1; s < 32; s <<= 1)
    g ^= g >> s;
  return g;
}

static unsigned bitrev(unsigned x) {
  x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
  x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
  x = ((x >> 4) & 0x0F0F0F0Fu) | ((x & 0x0F0F0F0Fu) << 4);
  x = ((x >> 8) & 0x00FF00FFu) | ((x & 0x00FF00FFu) << 8);
  return (x >> 16) | (x << 16);
}

static int clz(unsigned x) {
  if (!x) return 32;
  int n = 0;
  if (!(x & 0xFFFF0000u)) { n += 16; x <<= 16; }
  if (!(x & 0xFF000000u)) { n += 8; x <<= 8; }
  if (!(x & 0xF0000000u)) { n += 4; x <<= 4; }
  if (!(x & 0xC0000000u)) { n += 2; x <<= 2; }
  if (!(x & 0x80000000u)) { n += 1; }
  return n;
}

static int ctz(unsigned x) {
  if (!x) return 32;
  int n = 0;
  while (!(x & 1u)) { x >>= 1; n++; }
  return n;
}

static unsigned morton(unsigned short x, unsigned short y) {
  unsigned r = 0;
  for (int i = 0; i < 16; i++)
    r |= ((x >> i) & 1u) << (2 * i) | ((y >> i) & 1u) << (2 * i + 1);
  return r;
}

static unsigned next_pow2(unsigned x) {
  x--;
  x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8; x |= x >> 16;
  return x + 1;
}

int main(void) {
  unsigned vals[] = {0u, 1u, 2u, 3u, 0x80000000u, 0xFFFFFFFFu, 0x12345678u, 0xDEADBEEFu,
                     0x0F0F0F0Fu, 1000000007u, 0x7FFFFFFFu, 0xAAAAAAAAu};
  for (unsigned i = 0; i < sizeof vals / sizeof vals[0]; i++) {
    unsigned v = vals[i];
    ks_put_hex(v);
    ks_put_str(" pop=");
    ks_put_int(popcount(v));
    ks_put_str(popcount(v) == popcount_slow(v) ? "" : "!");
    ks_put_str(" par=");
    ks_put_int(popcount(v) & 1);
    ks_put_str(" clz=");
    ks_put_int(clz(v));
    ks_put_str(" ctz=");
    ks_put_int(ctz(v));
    ks_put_str(" rev=");
    ks_put_hex(bitrev(v));
    ks_put_str(" rl7=");
    ks_put_hex(rotl(v, 7));
    ks_put_str(" rr13=");
    ks_put_hex(rotr(v, 13));
    ks_put_str(" low=");
    ks_put_hex(v & (0u - v));
    ks_put_str(" p2=");
    ks_put_int(v && !(v & (v - 1)));
    ks_put_str(" np2=");
    ks_put_hex(next_pow2(v));
    ks_put_chr('\n');
  }
  int ok = 1;
  for (unsigned i = 0; i < 1000; i++) {
    unsigned g = gray(i);
    if (gray_inv(g) != i) ok = 0;
    if (popcount(g ^ gray(i + 1)) != 1) ok = 0;
    if (rotr(rotl(i * 2654435761u, (int)i), (int)i) != i * 2654435761u) ok = 0;
  }
  ks_put_kv("gray/rot ok", ok);
  for (unsigned i = 0; i < 8; i++) {
    ks_put_hex(gray(i));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kx("morton(3,5)", morton(3, 5));
  ks_put_kx("morton(ffff,0)", morton(0xFFFF, 0));
  ks_put_kx("morton(1234,abcd)", morton(0x1234, 0xABCD));
  // Extração e inserção de campos de bits
  unsigned w = 0;
  for (int f = 0; f < 8; f++)
    w = (w & ~(0xFu << (4 * f))) | ((unsigned)(f * 3 & 0xF) << (4 * f));
  ks_put_kx("fields", w);
  int sx = (int)(w << 4) >> 28; // extensão de sinal do campo 6
  ks_put_kv("signed field", sx);
  return 0;
}
