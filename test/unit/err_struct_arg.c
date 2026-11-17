// expect-error: struct/union passada por valor
struct S { int a; };
int f(int x) { return x; }
int g(struct S s);
int main() { struct S s; s.a = 1; return g(s); }
