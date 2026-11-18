// expect-error: conversao invalida entre ponteiro e ponto flutuante
// a[f]: subscrito de ponto flutuante (C11 6.5.2.1p1); virava FTOU do
// deslocamento escalado.
int a[4];
int main() { float f = 1.0f; return a[f]; }
