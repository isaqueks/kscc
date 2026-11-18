// expect-error: expressao constante inteira esperada
int main() { int x = 1; switch (x) { case 1.5: return 1; } return 0; }
