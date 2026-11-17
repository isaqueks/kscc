// Array com mais de INT_MAX bytes estourava o int de Type::size em
// array_of (UB no compilador, pego pelo UBSan em make test-asan).
// expect-error: array grande demais
int main() { return sizeof(int[1000000000]) != 0; }
