// expect-error: conversao invalida entre ponteiro e ponto flutuante
// ponteiro inicializado com float (C11 6.5.16.1p1): virava FTOU.
int main() { float f = 1.0f; int *p = f; return p != 0; }
