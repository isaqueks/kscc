// expect-error: VLA nao suportado
int main() { int n = 3; int a[n]; a[0] = 1; return a[0]; }
