// Valores negativos junto com valores acima de INT_MAX não cabem em 32
// bits (o gcc usaria long long); antes eram truncados em silêncio.
// expect-error: nao cabem em int nem em unsigned int
enum E { NEG = -1, BIG = 0x80000000u };
int main() { return BIG > 0; }
