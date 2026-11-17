// Structs aninhadas passadas e devolvidas por valor (tamanhos 1..40),
// atribuição de struct, arrays de structs, cópias independentes e literais
// compostos.
#include "kstest.h"

typedef struct { char c; } S1;
typedef struct { short a; char b; } S4;
typedef struct { char a, b, c; } S3;
typedef struct { int x, y; } Pt;
typedef struct { Pt min, max; } Rect;
typedef struct {
  char name[10];
  Rect r;
  short id;
  unsigned char flags[3];
} Obj;

static S1 s1_inc(S1 s) { s.c++; return s; }
static S3 s3_rot(S3 s) { S3 r = {s.b, s.c, s.a}; return r; }
static S4 s4_make(int a, int b) { return (S4){(short)a, (char)b}; }
static Pt pt_add(Pt a, Pt b) { return (Pt){a.x + b.x, a.y + b.y}; }

static Rect rect_union(Rect a, Rect b) {
  Rect r;
  r.min.x = a.min.x < b.min.x ? a.min.x : b.min.x;
  r.min.y = a.min.y < b.min.y ? a.min.y : b.min.y;
  r.max.x = a.max.x > b.max.x ? a.max.x : b.max.x;
  r.max.y = a.max.y > b.max.y ? a.max.y : b.max.y;
  return r;
}

static int area(Rect r) { return (r.max.x - r.min.x) * (r.max.y - r.min.y); }

static Obj obj_move(Obj o, Pt d, int n_args_after, int a5, int a6) {
  o.r.min = pt_add(o.r.min, d);
  o.r.max = pt_add(o.r.max, d);
  o.id = (short)(o.id + n_args_after + a5 + a6);
  o.flags[1] ^= 0xFF;
  o.name[0] = 'M';
  return o;
}

static void print_obj(const char *label, Obj o) {
  ks_put_str(label);
  ks_put_str(": ");
  ks_put_str(o.name);
  ks_put_str(" [");
  ks_put_int(o.r.min.x); ks_put_chr(',');
  ks_put_int(o.r.min.y); ks_put_chr(' ');
  ks_put_int(o.r.max.x); ks_put_chr(',');
  ks_put_int(o.r.max.y);
  ks_put_str("] id=");
  ks_put_int(o.id);
  ks_put_str(" flags=");
  for (int i = 0; i < 3; i++) {
    ks_put_int(o.flags[i]);
    ks_put_chr(i < 2 ? '/' : '\n');
  }
}

// Struct grande devolvida por valor e usada direto como argumento.
static Obj make_obj(const char *name, int id) {
  Obj o = {{0}, {{id, id + 1}, {id + 10, id + 20}}, (short)id, {1, 2, 3}};
  for (int i = 0; i < 9 && name[i]; i++)
    o.name[i] = name[i];
  return o;
}

static int sum_pts(int n, Pt *ps) {
  int s = 0;
  for (int i = 0; i < n; i++)
    s += ps[i].x * 10 + ps[i].y;
  return s;
}

int main(void) {
  S1 a = {'a'};
  a = s1_inc(s1_inc(a));
  ks_put_kv("s1", a.c);
  S3 t = {1, 2, 3};
  t = s3_rot(s3_rot(t));
  ks_put_kv("s3", t.a * 100 + t.b * 10 + t.c);
  S4 f = s4_make(-1234, 'z');
  ks_put_kv("s4.a", f.a);
  ks_put_kv("s4.b", f.b);
  ks_put_kv("sizeof S1 S3 S4", (int)(sizeof(S1) * 100 + sizeof(S3) * 10 + sizeof(S4)));
  ks_put_kv("sizeof Obj", (int)sizeof(Obj));
  Rect r1 = {{0, 0}, {4, 5}}, r2 = {{-3, 2}, {2, 9}};
  Rect u = rect_union(r1, r2);
  ks_put_kv("area", area(u));
  Obj o = make_obj("box", 7);
  Obj copy = o;
  Obj moved = obj_move(o, (Pt){100, -100}, 1, 2, 3);
  print_obj("o", o);
  print_obj("copy", copy);
  print_obj("moved", moved);
  print_obj("direct", obj_move(make_obj("tmp", 1), pt_add((Pt){1, 1}, (Pt){2, 2}), 0, 0, 0));
  Pt ps[5];
  for (int i = 0; i < 5; i++)
    ps[i] = (Pt){i, i * i};
  Pt tmp = ps[0];
  ps[0] = ps[4];
  ps[4] = tmp;
  ks_put_kv("sum_pts", sum_pts(5, ps));
  Obj arr[3];
  for (int i = 0; i < 3; i++)
    arr[i] = make_obj(i == 1 ? "mid" : "end", i * 3);
  arr[0] = arr[1];
  arr[1].name[0] = 'X';
  print_obj("arr0", arr[0]);
  print_obj("arr1", arr[1]);
  // struct com membro struct atribuído encadeado
  Rect q, w;
  q = w = r2;
  w.min.x = 99;
  ks_put_kv("q.min.x", q.min.x);
  ks_put_kv("w.min.x", w.min.x);
  // ponteiro para membro de struct aninhada
  int *py = &u.max.y;
  *py += 1000;
  ks_put_kv("u.max.y", u.max.y);
  return 0;
}
