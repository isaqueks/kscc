// Entrada padrão redirecionada pelo harness (// stdin:): getchar,
// fgets com e sem truncamento, gets_s (sem o '\n', truncando linha
// longa e descartando o resto) e EOF no fim.
#include <stdio.h>
#include <string.h>
#include "libks_test.h"
// stdin: ab
// stdin: linha um
// stdin: 0123456789
// stdin: curta
// stdin: muito comprida para o buffer
// stdin: depois
// stdin:
// stdin: fim
// expect-out: prompt> lido: 'a'
int main() {
  char buf[8];

  // getchar descarrega stdout antes de ler (o prompt aparece)
  printf("prompt> ");
  int c = getchar();
  printf("lido: '%c'\n", c);
  EQ('b', getchar());
  EQ('\n', getchar());

  // fgets inclui o '\n' quando cabe
  char line[32];
  EQ((int)line, (int)fgets(line, sizeof(line), stdin));
  STR_EQ("linha um\n", line);

  // fgets lê no máximo n-1 bytes; o resto fica para a próxima
  fgets(buf, 5, stdin);
  STR_EQ("0123", buf);
  fgets(buf, 8, stdin);
  STR_EQ("456789\n", buf);

  // gets_s tira o '\n'; linha longa é truncada e o resto descartado
  EQ((int)buf, (int)gets_s(buf, sizeof(buf)));
  STR_EQ("curta", buf);
  gets_s(buf, sizeof(buf));
  STR_EQ("muito c", buf);
  gets_s(buf, sizeof(buf));
  STR_EQ("depois", buf);
  gets_s(buf, sizeof(buf));  // linha vazia
  STR_EQ("", buf);

  // Última linha, depois EOF
  EQ('f', fgetc(stdin));
  EQ('i', getc(stdin));
  STR_EQ("m\n", fgets(line, sizeof(line), stdin));
  EQ(EOF, getchar());
  EQ(EOF, getchar());  // continua EOF
  EQ(0, (int)fgets(line, sizeof(line), stdin));
  EQ(0, (int)gets_s(buf, sizeof(buf)));
  EQ(EOF, fgetc(stdout));  // só stdin é legível
  return 0;
}
