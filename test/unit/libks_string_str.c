// Funções de string do <string.h>: comprimento, cópia, concatenação,
// comparação e busca, com as bordas de cada uma.
#include <stdlib.h>
#include <string.h>
#include "libks_test.h"

int main() {
  char buf[32];

  EQ(0, strlen(""));
  EQ(5, strlen("hello"));
  EQ(3, strnlen("abc", 10));
  EQ(2, strnlen("abc", 2));
  EQ(0, strnlen("abc", 0));

  EQ((int)buf, (int)strcpy(buf, "abc"));
  STR_EQ("abc", buf);
  strcpy(buf, "");
  EQ(0, buf[0]);

  // strncpy: completa com '\0'; sem '\0' se a origem não cabe
  memset(buf, 'X', sizeof(buf));
  strncpy(buf, "ab", 5);
  EQ('a', buf[0]);
  EQ(0, buf[2]);
  EQ(0, buf[4]);
  EQ('X', buf[5]);
  memset(buf, 'X', sizeof(buf));
  strncpy(buf, "abcdef", 3);
  EQ('c', buf[2]);
  EQ('X', buf[3]);

  strcpy(buf, "ab");
  EQ((int)buf, (int)strcat(buf, "cd"));
  STR_EQ("abcd", buf);
  strcat(buf, "");
  STR_EQ("abcd", buf);
  strncat(buf, "efgh", 2);
  STR_EQ("abcdef", buf);
  strncat(buf, "xy", 10);
  STR_EQ("abcdefxy", buf);
  strncat(buf, "zzz", 0);
  STR_EQ("abcdefxy", buf);

  EQ(0, strcmp("", ""));
  EQ(0, strcmp("abc", "abc"));
  EQ(1, strcmp("abc", "abd") < 0);
  EQ(1, strcmp("abd", "abc") > 0);
  EQ(1, strcmp("ab", "abc") < 0);
  EQ(1, strcmp("abc", "ab") > 0);
  EQ(1, strcmp("a", "\xe9") < 0);  // bytes comparados sem sinal
  EQ(0, strncmp("abcx", "abcy", 3));
  EQ(1, strncmp("abcx", "abcy", 4) < 0);
  EQ(0, strncmp("abc", "abd", 0));
  EQ(0, strncmp("ab", "ab\0zz", 5));

  const char *s = "banana";
  EQ(2, strchr(s, 'n') - s);
  EQ(4, strrchr(s, 'n') - s);
  EQ(0, (int)strchr(s, 'z'));
  EQ(0, (int)strrchr(s, 'z'));
  EQ(6, strchr(s, 0) - s);  // o '\0' final conta
  EQ(6, strrchr(s, 0) - s);
  EQ(0, strrchr(s, 'b') - s);

  EQ(2, strstr(s, "nan") - s);
  EQ(0, strstr(s, "") - s);
  EQ(0, (int)strstr(s, "nab"));
  EQ(0, (int)strstr("", "a"));
  EQ(4, strstr(s, "na\0x") - s + 2);  // a agulha termina no '\0'
  const char *t = "aaab";
  EQ(1, strstr(t, "aab") - t);  // recomeça depois de um casamento parcial

  char *d = strdup("copia");
  EQ(1, d != 0 && d != (char *)"copia");
  STR_EQ("copia", d);
  d[0] = 'C';
  STR_EQ("Copia", d);
  free(d);
  d = strdup("");
  EQ(0, d[0]);
  free(d);
  return 0;
}
