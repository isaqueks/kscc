// expect-error: VLA com alinhamento > 4
// alloca só garante alinhamento 4 (doc 03 §2).
struct A { _Alignas(16) int x; };
int main() {
  int n = 2;
  struct A a[n];
  a[0].x = 1;
  return a[0].x;
}
