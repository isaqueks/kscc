// Identificadores não ASCII e com '$' em símbolos globais: o ksas só
// aceita [A-Za-z_][A-Za-z0-9_.]*, então o codegen escapa os bytes
// (`π` -> `_.cf.80`). Inclui função e static com nome não ASCII.
#include "minirt.h"
int π = 3;
int $dólar = 4;
static int ação(int x) { return x * 2; }
int été = 5; // UCN: mesmo símbolo que "été"

int main() {
  EQ(3, π);
  EQ(4, $dólar);
  EQ(10, ação(5));
  EQ(5, été);
  int (*f)(int) = ação;
  EQ(14, f(7));
  return 0;
}
