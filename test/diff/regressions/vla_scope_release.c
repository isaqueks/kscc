#include "kstest.h"
// O VLA é liberado no fim do bloco que o declara (C11 6.2.4p7), também
// quando o bloco é deixado por continue, break ou goto. Antes só o
// epílogo liberava, e um VLA num laço consumia pilha a cada iteração até
// FAULT_STACK (aqui, centenas de KB numa pilha de 16 KB).
// kscc-flags: -Wl,--stack=16384
static int f6(int a, int b, int c, int d, int e, int f) {
  return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f;
}

int main(void) {
  int n = 64, i, s = 0;

  for (i = 0; i < 2000; i++) {   // saída normal do bloco
    int v[n];
    v[i % n] = i;
    s += v[i % n];
  }
  ks_put_kv("normal", s);

  s = 0;
  for (i = 0; i < 2000; i++) {   // continue
    int v[n];
    v[0] = i;
    if (i & 1)
      continue;
    s += v[0];
  }
  ks_put_kv("continue", s);

  s = 0;
  int outer[n];                   // VLA externo continua intacto
  for (int k = 0; k < n; k++)
    outer[k] = k * 3;
  for (i = 0; i < 2000; i++) {   // goto através de dois blocos com VLA
    int v[n];
    v[1] = i;
    {
      char w[n * 4];
      w[0] = (char)i;
      if (i % 3 == 0)
        goto next;
      s += w[0] & 1;
    }
    s += v[1] & 7;
  next:;
  }
  ks_put_kv("goto", s);
  ks_put_kv("outer", outer[n - 1] + outer[1]);

  s = 0;
  for (i = 0; i < 2000; i++) {   // break de um switch dentro do bloco
    int v[n];
    v[2] = i;
    switch (i % 4) {
    case 0: {
      int w[n];
      w[0] = v[2];
      s += w[0] & 3;
      break;
    }
    default:
      s += 1;
    }
  }
  ks_put_kv("switch", s);

  // Bloco com VLA dentro de uma expressão-comando avaliada com
  // temporários na pilha (f6 já empilhou argumentos; x + ... empilhou x).
  s = 0;
  for (i = 0; i < 2000; i++) {
    int x = i & 15;
    s += x + ({ int r = 0; { int v[n]; v[3] = i & 7; r = v[3]; } r; });
    s += f6(1, 2, 3, 4, 5, ({ int r; { int v[n]; v[0] = 6; r = v[0]; } r; }));
  }
  ks_put_kv("stmt-expr", s);

  // Saída por goto de dentro de um bloco com VLA dentro de uma
  // expressão-comando com temporário empilhado.
  s = 0;
  for (i = 0; i < 2000; i++) {
    s += 1 + ({ { int v[n]; v[0] = i; if (v[0] & 1) goto skip; } 2; });
  skip:;
  }
  ks_put_kv("goto stmt-expr", s);

  // Laço externo sem VLA, interno com: o break sai só do interno.
  s = 0;
  for (i = 0; i < 300; i++) {
    for (int j = 0; ; j++) {
      int v[n];
      v[j % n] = j;
      if (j == 5)
        break;
      s += v[j % n];
    }
  }
  ks_put_kv("break", s);
  return 0;
}
