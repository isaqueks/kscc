// expect-error: retorno de struct/union
struct S { int a; };
struct S f(void) { struct S s; s.a = 1; return s; }
int main() { return 0; }
