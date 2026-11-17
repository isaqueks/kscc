// Bitfields: signed/unsigned, larguras variadas, campos que cruzam bytes,
// campos de largura 0, compostos (+=, ++), wrap de unsigned, sizeof e a
// representação em memória (lida por union, little-endian nos dois alvos).
#include "kstest.h"

struct Flags {
  unsigned ready : 1;
  unsigned mode : 3;
  unsigned count : 12;
  int delta : 5;
  unsigned : 0;
  unsigned high : 7;
  signed char tag;
};

struct Packed {
  unsigned char a : 4, b : 4;
  unsigned short c : 9, d : 7;
};

struct Wide {
  unsigned lo : 20;
  unsigned hi : 12;
  int s : 31;
  unsigned bit : 1;
};

union View {
  struct Flags f;
  unsigned w[3];
};

union PView {
  struct Packed p;
  unsigned char bytes[4];
};

static int get_delta(struct Flags f) { return f.delta; }
static struct Flags bump(struct Flags f) {
  f.count++;
  f.mode += 3;
  f.delta -= 3;
  return f;
}

int main(void) {
  ks_put_kv("sizeof Flags", (int)sizeof(struct Flags));
  ks_put_kv("sizeof Packed", (int)sizeof(struct Packed));
  ks_put_kv("sizeof Wide", (int)sizeof(struct Wide));
  union View v = {{0}};
  v.f.ready = 1;
  v.f.mode = 5;
  v.f.count = 4095;
  v.f.delta = -16;
  v.f.high = 100;
  v.f.tag = -3;
  ks_put_kx("w0", v.w[0]);
  ks_put_kx("w1", v.w[1]);
  ks_put_kv("delta", get_delta(v.f));
  struct Flags f = v.f;
  for (int i = 0; i < 5; i++) {
    f = bump(f);
    ks_put_str("count=");
    ks_put_int(f.count);
    ks_put_str(" mode=");
    ks_put_int(f.mode);
    ks_put_str(" delta=");
    ks_put_int(f.delta);
    ks_put_str(" ready=");
    ks_put_int(f.ready);
    ks_put_chr('\n');
  }
  f.count += 100; // wrap em 12 bits
  ks_put_kv("count+100", f.count);
  f.ready = 0;
  f.ready ^= 1;
  ks_put_kv("ready", f.ready);
  ks_put_kv("f.mode * 2 promovido", f.mode * 2);
  ks_put_kv("-f.count < 0", -f.count < 0); // unsigned:12 promove a int
  union PView p = {{0}};
  p.p.a = 0xA;
  p.p.b = 0x5;
  p.p.c = 0x1FF;
  p.p.d = 0x2A;
  for (int i = 0; i < 4; i++) {
    ks_put_hex(p.bytes[i]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  struct Wide w = {0xABCDE, 0x123, -123456789, 1};
  ks_put_kx("lo", w.lo);
  ks_put_kx("hi", w.hi);
  ks_put_kv("s", w.s);
  ks_put_kv("bit", w.bit);
  w.s = w.s / 2 - 1;
  w.lo += w.hi;
  ks_put_kv("s'", w.s);
  ks_put_kx("lo'", w.lo);
  // array de structs com bitfields
  struct Packed arr[6];
  for (int i = 0; i < 6; i++) {
    arr[i].a = (unsigned char)(i * 5);
    arr[i].b = (unsigned char)(15 - i);
    arr[i].c = (unsigned short)(i * 100);
    arr[i].d = (unsigned short)(i * 30);
  }
  int s = 0;
  for (int i = 0; i < 6; i++)
    s = s * 7 + arr[i].a + arr[i].b * 3 + arr[i].c + arr[i].d;
  ks_put_kv("sum", s);
  return 0;
}
