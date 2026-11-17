// Unions com punning de bytes (little-endian nos dois alvos): int/short/char,
// troca de endianness, union dentro de struct, tagged union e unions
// devolvidas por valor.
#include "kstest.h"

union Word {
  unsigned u;
  int i;
  unsigned short h[2];
  unsigned char b[4];
  signed char sb[4];
};

static union Word make(unsigned x) {
  union Word w;
  w.u = x;
  return w;
}

static unsigned bswap(unsigned x) {
  union Word in = make(x), out;
  for (int i = 0; i < 4; i++)
    out.b[i] = in.b[3 - i];
  return out.u;
}

enum Kind { K_INT, K_PAIR, K_TEXT };
typedef struct {
  enum Kind kind;
  union {
    int i;
    struct { short a, b; } pair;
    char text[8];
  } as;
} Value;

static int value_hash(Value v) {
  switch (v.kind) {
  case K_INT: return v.as.i;
  case K_PAIR: return v.as.pair.a * 1000 + v.as.pair.b;
  case K_TEXT: {
    int h = 0;
    for (int i = 0; i < 8 && v.as.text[i]; i++)
      h = h * 31 + v.as.text[i];
    return h;
  }
  }
  return -1;
}

static void put_bytes(const char *label, union Word w) {
  ks_put_str(label);
  for (int i = 0; i < 4; i++) {
    ks_put_chr(' ');
    ks_put_int(w.b[i]);
  }
  ks_put_str(" |");
  for (int i = 0; i < 4; i++) {
    ks_put_chr(' ');
    ks_put_int(w.sb[i]);
  }
  ks_put_str(" | h0=");
  ks_put_int(w.h[0]);
  ks_put_str(" h1=");
  ks_put_int(w.h[1]);
  ks_put_str(" i=");
  ks_put_int(w.i);
  ks_put_chr('\n');
}

int main(void) {
  put_bytes("0x12345678", make(0x12345678u));
  put_bytes("0xFFFF8001", make(0xFFFF8001u));
  put_bytes("1", make(1));
  ks_put_kx("bswap", bswap(0xDEADBEEFu));
  ks_put_kx("bswap2", bswap(bswap(0x01020304u)));
  union Word w;
  w.u = 0;
  w.b[3] = 0x80;
  ks_put_kv("sign via byte", w.i < 0);
  w.h[0] = 0xFFFF;
  ks_put_kx("after h0", w.u);
  ks_put_kv("sizeof Word", (int)sizeof(union Word));
  Value vals[3];
  vals[0].kind = K_INT;
  vals[0].as.i = -42;
  vals[1].kind = K_PAIR;
  vals[1].as.pair.a = 7;
  vals[1].as.pair.b = -9;
  vals[2].kind = K_TEXT;
  for (int i = 0; i < 8; i++)
    vals[2].as.text[i] = "ksvm!"[i < 5 ? i : 5];
  for (int i = 0; i < 3; i++)
    ks_put_kv("hash", value_hash(vals[i]));
  // a pair visto como int: bytes de a nos 2 baixos
  ks_put_kx("pair as int", (unsigned)vals[1].as.i);
  ks_put_kv("sizeof Value", (int)sizeof(Value));
  // union com array de bytes usada como buffer de serialização
  union {
    struct { unsigned short len; unsigned char type, flags; unsigned crc; } hdr;
    unsigned char raw[8];
  } pkt;
  for (int i = 0; i < 8; i++)
    pkt.raw[i] = (unsigned char)(i * 37 + 11);
  ks_put_kv("len", pkt.hdr.len);
  ks_put_kv("type", pkt.hdr.type);
  ks_put_kv("flags", pkt.hdr.flags);
  ks_put_kx("crc", pkt.hdr.crc);
  pkt.hdr.crc = 0xCAFEBABEu;
  ks_put_kv("raw[4]", pkt.raw[4]);
  ks_put_kv("raw[7]", pkt.raw[7]);
  return 0;
}
