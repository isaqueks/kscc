// Um endereço não cabe num inicializador de char/short: antes virava um
// `.word x` que apagava os membros vizinhos (b, c, d) em silêncio.
// expect-error: initializer element is not constant
int x;
struct { char a, b, c, d; } s = { (char)(long)&x, 2, 3, 4 };
int main() { return s.b; }
