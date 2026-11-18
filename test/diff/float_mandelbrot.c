// float: conjunto de Mandelbrot em ASCII (contagem de iterações por ponto)
// e uma soma de verificação dos |z|^2 finais. Cada iteração depende do
// arredondamento exato da anterior: qualquer diferença de um bit muda a
// figura.
#include "fltout.h"

int main(void) {
  const char *shade = " .:-=+*#%@";
  unsigned hash = 0;
  for (int row = 0; row < 24; row++) {
    float ci = -1.2f + row * 0.1f;
    for (int col = 0; col < 64; col++) {
      float cr = -2.1f + col * 0.045f;
      float zr = 0, zi = 0, r2 = 0;
      int it = 0;
      while (it < 60) {
        float nzr = zr * zr - zi * zi + cr;
        zi = 2 * zr * zi + ci;
        zr = nzr;
        r2 = zr * zr + zi * zi;
        if (r2 > 4)
          break;
        it++;
      }
      putchar(shade[it % 10]);
      hash = hash * 31u + fl_bits(r2 == r2 ? r2 : 0) + (unsigned)it;
    }
    putchar('\n');
  }
  printf("hash = %08x\n", hash);
  return 0;
}
