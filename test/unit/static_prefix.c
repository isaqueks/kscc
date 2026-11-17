// static vira _S<id>_nome; um extern que se refere ao static também.
#include "minirt.h"
static int hidden = 3;
static int helper(int x) { return x + hidden; }
int main() {
  extern int hidden;
  EQ(7, helper(4));
  EQ(3, hidden);
  return 0;
}
