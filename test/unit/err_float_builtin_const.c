// expect-error: not a compile-time constant
// O kscc não dobra os builtins de ponto flutuante em constantes (doc 03 §1).
static float r = __builtin_sqrtf(2);
int main() { return (int)r; }
