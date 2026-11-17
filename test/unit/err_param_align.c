// expect-error: parametro struct/union com alinhamento > 4
// A cópia de um argumento struct na pilha só é alinhada a 4.
struct A { _Alignas(8) int x; };
int f(struct A a) { return a.x; }
int main() { return 0; }
