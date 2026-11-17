// Escalar passado a parâmetro struct era aceito: ia em r1 e o chamado
// lia a struct da pilha (lixo em silêncio). O contrário passava o
// endereço da struct.
// expect-error: incompatible type for argument
struct S { int a, b; };
int f(struct S s) { return s.a; }
int main() { return f(5); }
