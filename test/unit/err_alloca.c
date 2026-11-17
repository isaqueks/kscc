// expect-error: alloca nao suportado
int main() { char *p = alloca(10); return p != 0; }
