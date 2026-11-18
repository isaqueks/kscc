// float em dados: globais e static inicializados (arrays, structs aninhadas,
// unions, designadores), tabelas const em .rodata, ponteiros para floats
// globais, e cópia de structs com floats.
#include "fltout.h"

typedef struct { const char *name; float k; float range[2]; } Entry;
typedef union { float f; unsigned u; unsigned char b[4]; } Pun;

static const Entry table[] = {
  {"agua", 4.186f, {0, 100}},
  {"ferro", 0.449f, {-50.5f, 1538}},
  {"ar", 1.005f, {[1] = 3.5e3f}},
  {.name = "vidro", .k = 0.84f, .range = {-1e-3f, 1e3f}},
};
static float grid[3][4] = {{1, 2, 3, 4}, {0.5f, 0.25f, 0.125f}, [2][3] = -1e30f};
static Pun puns[] = {{1.0f}, {.u = 0x40490FDB}, {.b = {0, 0, 0x80, 0x7F}}};
static float *ptrs[] = {&grid[0][1], &grid[1][2], &grid[2][3]};
float extern_val = 2.5f;
static struct { int n; float avg; struct { float lo, hi; } lim; } stats = {3, 1.0f / 3, {-0.0f, 1e-45f}};

static Entry copy_entry(Entry e, float scale) {
  e.k *= scale;
  e.range[0] -= scale;
  return e;
}

int main(void) {
  for (int i = 0; i < 4; i++) {
    Entry e = copy_entry(table[i], 1.5f);
    printf("%-6s ", table[i].name);
    fl_hex(table[i].k);
    fl_hex(table[i].range[0]);
    fl_hex(table[i].range[1]);
    fl_hex(e.k);
    fl_hex(e.range[0]);
    printf("%.3f\n", e.k);
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++)
      fl_hex(grid[i][j]);
    printf("\n");
  }
  for (int i = 0; i < 3; i++) {
    printf("pun %08x ", puns[i].u);
    fl_hex(puns[i].f);
    printf("%d %d %d %d\n", puns[i].b[0], puns[i].b[1], puns[i].b[2], puns[i].b[3]);
  }
  for (int i = 0; i < 3; i++) {
    *ptrs[i] *= 3;
    fl_hex(*ptrs[i]);
  }
  printf("\n");
  fl_put("extern", extern_val);
  fl_put("avg", stats.avg);
  fl_put("lo", stats.lim.lo);
  fl_put("hi", stats.lim.hi);
  static float counter = 0.5f;
  for (int i = 0; i < 5; i++)
    counter = counter * counter + 0.25f;
  fl_put("counter", counter);
  return 0;
}
