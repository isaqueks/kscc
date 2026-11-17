// Divisão por zero em tempo de execução: FAULT_DIV_ZERO (código 70 + 6).
// expect-exit: 76
int main() {
  int z = 0;
  return 10 / z;
}
