// Literal de string fica em .rodata (região de código, só leitura).
// expect-exit: 75
int main() {
  char *s = "abc";
  s[0] = 'x';
  return 0;
}
