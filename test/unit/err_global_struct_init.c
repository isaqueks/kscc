// expect-error: initializer element is not constant
// O chibicc deixava g2 zerada em silêncio.
struct T { int a; };
struct T g1 = {1};
struct T g2 = g1;
int main() { return g2.a; }
