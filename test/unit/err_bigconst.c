// 2147483648 decimal é long long na ILP32.
// expect-error: nao cabe em 32 bits
int main() { return 2147483648 > 0; }
