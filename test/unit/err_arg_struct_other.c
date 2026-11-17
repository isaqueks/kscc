// Struct passada a parâmetro escalar (ou de outro tipo struct) era
// aceita em silêncio.
// expect-error: incompatible type for argument
struct S { int a, b; };
struct T { int a, b; };
int g(int x) { return x; }
int h(struct T t) { return t.a; }
int main() { struct S s = {1, 2}; return g(1) + h(s); }
