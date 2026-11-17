// Largura de bit-field maior que o tipo era aceita e gerava shli com
// contagem negativa (C11 6.7.2.1p4 exige diagnóstico).
// expect-error: largura de bit-field invalida
struct S { int x : 33; };
int main() { struct S s; s.x = 1; return s.x; }
