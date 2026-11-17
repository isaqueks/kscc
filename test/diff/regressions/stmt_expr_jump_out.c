#include "kstest.h"
// Saltos (continue, break, goto) para fora de uma expressão-comando com
// temporários na pilha. Sem desempilhá-los, cada iteração deixava 4 bytes
// por temporário e o laço estourava a pilha (FAULT_STACK).
static int f6(int a, int b, int c, int d, int e, int f) {
  return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f;
}
int main(void) {
  int s = 0, u = 0, w = 0, i;
  unsigned t = 0;
  for (i = 0; i < 100000; i++)
    s += 1 + ({ if (i & 1) continue; 2; });
  ks_put_kv("continue", s);

  for (i = 0; i < 100000; i++)
    t = t * 3 + ({ if (i > 50000) goto out; (unsigned)i; });
out:
  ks_put_kx("goto", t);

  for (i = 0; i < 30000; i++) {
    switch (i % 3) {
    case 0:
      u += 7 * ({ if (i % 2) break; 1; });
      break;
    default:
      u += f6(i, 2, 3, 4, ({ if (i % 5 == 0) break; 5; }), 6);
    }
  }
  ks_put_kv("switch", u);

  // Label dentro da própria expressão-comando, com temporário de fora
  // empilhado: o salto para trás não desempilha nada.
  for (i = 0; i < 1000; i++)
    w += i * ({
      int k = 0;
    again:
      k++;
      if (k < 3)
        goto again;
      for (int j = 0; j < 10; j++)
        k += 100 + ({ if (j == 4) break; j; });
      k;
    });
  ks_put_kv("nested", w);

  // Aninhadas: o continue sai das duas de uma vez.
  int v = 0;
  for (i = 0; i < 50000; i++)
    v += 3 * ({ 5 + ({ if (i & 2) continue; i & 7; }); });
  ks_put_kv("nested2", v);
  return 0;
}
