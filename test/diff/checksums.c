// Checksums e hashes sobre bytes: CRC-32 (tabela gerada em tempo de
// execução), CRC-16/CCITT bit a bit, Adler-32, FNV-1a, Fletcher-16,
// murmur3 finalizer e base64.
#include "kstest.h"

static unsigned crc_table[256];

static void crc32_init(void) {
  for (unsigned i = 0; i < 256; i++) {
    unsigned c = i;
    for (int k = 0; k < 8; k++)
      c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
    crc_table[i] = c;
  }
}

static unsigned crc32(const unsigned char *p, int n) {
  unsigned c = 0xFFFFFFFFu;
  while (n--)
    c = crc_table[(c ^ *p++) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

static unsigned short crc16(const unsigned char *p, int n) {
  unsigned short crc = 0xFFFF;
  for (int i = 0; i < n; i++) {
    crc ^= (unsigned short)(p[i] << 8);
    for (int k = 0; k < 8; k++)
      crc = crc & 0x8000 ? (unsigned short)((crc << 1) ^ 0x1021) : (unsigned short)(crc << 1);
  }
  return crc;
}

static unsigned adler32(const unsigned char *p, int n) {
  unsigned a = 1, b = 0;
  for (int i = 0; i < n; i++) {
    a = (a + p[i]) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

static unsigned fnv1a(const unsigned char *p, int n) {
  unsigned h = 2166136261u;
  for (int i = 0; i < n; i++)
    h = (h ^ p[i]) * 16777619u;
  return h;
}

static unsigned short fletcher16(const unsigned char *p, int n) {
  unsigned short s1 = 0, s2 = 0;
  for (int i = 0; i < n; i++) {
    s1 = (unsigned short)((s1 + p[i]) % 255);
    s2 = (unsigned short)((s2 + s1) % 255);
  }
  return (unsigned short)((s2 << 8) | s1);
}

static unsigned fmix32(unsigned h) {
  h ^= h >> 16;
  h *= 0x85ebca6bu;
  h ^= h >> 13;
  h *= 0xc2b2ae35u;
  h ^= h >> 16;
  return h;
}

static int base64(const unsigned char *in, int n, char *out) {
  static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int o = 0;
  for (int i = 0; i < n; i += 3) {
    unsigned v = (unsigned)in[i] << 16;
    if (i + 1 < n) v |= (unsigned)in[i + 1] << 8;
    if (i + 2 < n) v |= in[i + 2];
    out[o++] = tbl[(v >> 18) & 63];
    out[o++] = tbl[(v >> 12) & 63];
    out[o++] = i + 1 < n ? tbl[(v >> 6) & 63] : '=';
    out[o++] = i + 2 < n ? tbl[v & 63] : '=';
  }
  out[o] = 0;
  return o;
}

static int slen(const char *s) {
  int n = 0;
  while (s[n]) n++;
  return n;
}

int main(void) {
  crc32_init();
  const char *msgs[] = {"", "a", "abc", "123456789", "The quick brown fox jumps over the lazy dog",
                        "KSVM v2: maquina virtual de 32 bits\n"};
  char b64[128];
  for (int i = 0; i < 6; i++) {
    const unsigned char *p = (const unsigned char *)msgs[i];
    int n = slen(msgs[i]);
    ks_put_hex(crc32(p, n));
    ks_put_chr(' ');
    ks_put_hex(crc16(p, n));
    ks_put_chr(' ');
    ks_put_hex(adler32(p, n));
    ks_put_chr(' ');
    ks_put_hex(fnv1a(p, n));
    ks_put_chr(' ');
    ks_put_hex(fletcher16(p, n));
    ks_put_chr(' ');
    base64(p, n < 30 ? n : 30, b64);
    ks_put_str(b64);
    ks_put_chr('\n');
  }
  unsigned char bin[256];
  for (int i = 0; i < 256; i++)
    bin[i] = (unsigned char)(255 - i);
  ks_put_kx("crc32(bin)", crc32(bin, 256));
  ks_put_kx("adler(bin)", adler32(bin, 256));
  unsigned h = 0;
  for (unsigned i = 0; i < 1000; i++)
    h += fmix32(i);
  ks_put_kx("fmix", h);
  return 0;
}
