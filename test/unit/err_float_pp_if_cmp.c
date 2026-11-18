// expect-error: ponto flutuante em expressao de #if
// O tipo do topo é int, mas o operando é float (C11 6.10.1p1).
#if 1.5 > 1
#endif
int main() { return 0; }
