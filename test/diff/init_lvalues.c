// Inicializadores (designados, aninhados, parciais, strings em arrays de
// char), endereços em globais (aritmética, strings, tabelas de funções) e
// lvalues com efeito colateral avaliados uma vez só (a[i++] += k, *p++ op=).
#include "kstest.h"

struct In { short s; char c[3]; };
struct Out {
  int id;
  struct In in[2];
  const char *name;
  unsigned flags;
};

static int arr[10] = {[2] = 20, [5] = 50, 60, [9] = 90};
static int *parr = &arr[3] + 2;
static const char *str = "abcdef" + 2;
static char small[4] = "abc";
static char exact[3] = "xyz";      // sem '\0' (permitido em C)
static char big[8] = "hi";
static struct Out outs[3] = {
    [1] = {.id = 7, .in = {{1, {'a', 'b'}}, [1].s = -5}, .name = "um", .flags = 3u},
    {8, {{2, "cd"}}, "dois", 0xF0000000u},
};
static int sq(int x) { return x * x; }
static int neg(int x) { return -x; }
static int (*const ops[])(int) = {sq, neg, sq};
static struct { int (*fn)(int); int arg; } calls[] = {{neg, 4}, {sq, 9}};
static const int dims[] = {sizeof(struct In), sizeof(struct Out), sizeof outs / sizeof outs[0]};
static unsigned char wrap_init = (unsigned char)-1;
static short neg_init = (short)0x18000;
static const char *const words[] = {"zero", "um", [4] = "quatro"};

static int counter;
static int next(void) { return counter++; }

int main(void) {
  for (int i = 0; i < 10; i++) {
    ks_put_int(arr[i]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("*parr", *parr);
  ks_put_kv("parr - arr", (int)(parr - arr));
  ks_put_str(str);
  ks_put_chr('\n');
  ks_put_str(small);
  ks_put_kv(" exact[2]", exact[2]);
  ks_put_kv("big[5]", big[5]);
  for (int i = 0; i < 3; i++) {
    struct Out *o = &outs[i];
    ks_put_int(o->id);
    ks_put_chr(' ');
    ks_put_int(o->in[0].s);
    ks_put_chr(' ');
    ks_put_int(o->in[0].c[0]);
    ks_put_chr(' ');
    ks_put_int(o->in[0].c[1]);
    ks_put_chr(' ');
    ks_put_int(o->in[0].c[2]);
    ks_put_chr(' ');
    ks_put_int(o->in[1].s);
    ks_put_chr(' ');
    ks_put_str(o->name ? o->name : "(null)");
    ks_put_chr(' ');
    ks_put_hex(o->flags);
    ks_put_chr('\n');
  }
  int acc = 0;
  for (int i = 0; i < 3; i++)
    acc = acc * 100 + ops[i](i + 2);
  ks_put_kv("ops", acc);
  ks_put_kv("calls", calls[0].fn(calls[0].arg) * 1000 + calls[1].fn(calls[1].arg));
  ks_put_kv("dims", dims[0] * 10000 + dims[1] * 100 + dims[2]);
  ks_put_kv("wrap_init", wrap_init);
  ks_put_kv("neg_init", neg_init);
  for (int i = 0; i < 5; i++) {
    ks_put_str(words[i] ? words[i] : "-");
    ks_put_chr(' ');
  }
  ks_put_chr('\n');

  // locais: designados, parciais (resto zerado), aninhados, literais compostos
  int loc[6] = {[4] = 4, 1, [0] = 9};
  struct Out lo = {.name = "local", .in[1].c = {'q'}, .id = -1};
  ks_put_kv("loc", loc[0] * 100000 + loc[1] * 10000 + loc[4] * 10 + loc[5]);
  ks_put_kv("lo.in[1].c[0]", lo.in[1].c[0]);
  ks_put_kv("lo.in[1].c[1]", lo.in[1].c[1]);
  ks_put_kv("lo.flags", (int)lo.flags);
  ks_put_str(lo.name);
  ks_put_chr('\n');
  char s1[] = "multi" "part" " string";
  ks_put_str(s1);
  ks_put_kv(" sizeof", (int)sizeof s1);
  int *cl = (int[]){5, 6, 7};
  ks_put_kv("cl[2]", cl[2]);
  struct In *pin = &(struct In){.s = 99, .c = "z"};
  ks_put_kv("pin", pin->s + pin->c[0]);

  // lvalues com efeito colateral: o endereço é calculado uma vez
  int a[5] = {10, 20, 30, 40, 50};
  int i = 0;
  a[i++] += 5;
  a[i++] *= 3;
  a[next()] -= 1;
  a[next() + 2] <<= 2;
  int *p = a;
  *p++ += 100;
  *p++ |= 1;
  ks_put_kv("i", i);
  ks_put_kv("counter", counter);
  ks_put_kv("p - a", (int)(p - a));
  for (int k = 0; k < 5; k++) {
    ks_put_int(a[k]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  struct In ins[3] = {{1, "a"}, {2, "b"}, {3, "c"}};
  int j = 0;
  ins[j++].s += 10;
  ins[j++].c[0]++;
  ks_put_kv("ins", ins[0].s * 1000 + ins[1].c[0] * 10 + j);
  unsigned char uc = 250;
  int loops = 0;
  for (unsigned char c = uc; c != 4; c++)
    loops++;
  ks_put_kv("uchar loop", loops);
  return 0;
}
