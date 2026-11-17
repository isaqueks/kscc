// fizzbuzz.c — laços, resto e printf com largura.
//
// expect-out:  1 1
// expect-out:  2 2
// expect-out:  3 Fizz
// expect-out:  4 4
// expect-out:  5 Buzz
// expect-out:  6 Fizz
// expect-out:  7 7
// expect-out:  8 8
// expect-out:  9 Fizz
// expect-out: 10 Buzz
// expect-out: 11 11
// expect-out: 12 Fizz
// expect-out: 13 13
// expect-out: 14 14
// expect-out: 15 FizzBuzz
#include <stdio.h>

int main(void) {
  for (int i = 1; i <= 15; i++) {
    printf("%2d ", i);
    if (i % 15 == 0)
      puts("FizzBuzz");
    else if (i % 3 == 0)
      puts("Fizz");
    else if (i % 5 == 0)
      puts("Buzz");
    else
      printf("%d\n", i);
  }
  return 0;
}
