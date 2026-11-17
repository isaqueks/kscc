// Tamanho de array negativo era aceito em silêncio (o objeto ficava com
// tamanho negativo; numa definição provisória, virava 1 elemento).
// expect-error: size of array is negative
int arr[-1];
int main() { return 0; }
