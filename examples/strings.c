// strings.c — <string.h>, <ctype.h>, snprintf e qsort sobre palavras.
//
// expect-out: frase: "  o rato roeu a roupa do rei de roma  "
// expect-out: palavras: 9, maior: "roupa" (5)
// expect-out: ordenadas: a de do o rato rei roeu roma roupa
// expect-out: maiusculas: O RATO ROEU A ROUPA DO REI DE ROMA
// expect-out: "ro" aparece 3 vezes
// expect-out: truncado: "o rato r" (pedia 26)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 16

static int by_string(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}

// Divide `s` (modificada no lugar) em palavras separadas por espaço.
static int split(char *s, char **words) {
  int n = 0;
  for (;;) {
    while (isspace(*s))
      s++;
    if (!*s || n == MAX_WORDS)
      return n;
    words[n++] = s;
    while (*s && !isspace(*s))
      s++;
    if (*s)
      *s++ = '\0';
  }
}

static int count_occurrences(const char *s, const char *needle) {
  int n = 0;
  for (const char *p = strstr(s, needle); p; p = strstr(p + 1, needle))
    n++;
  return n;
}

int main(void) {
  const char *phrase = "  o rato roeu a roupa do rei de roma  ";
  printf("frase: \"%s\"\n", phrase);

  char *copy = strdup(phrase);
  char *words[MAX_WORDS];
  int n = split(copy, words);

  const char *longest = "";
  for (int i = 0; i < n; i++)
    if (strlen(words[i]) > strlen(longest))
      longest = words[i];
  printf("palavras: %d, maior: \"%s\" (%d)\n", n, longest, (int)strlen(longest));

  qsort(words, n, sizeof(char *), by_string);
  printf("ordenadas:");
  for (int i = 0; i < n; i++)
    printf(" %s", words[i]);
  printf("\n");

  char upper[64];
  strcpy(upper, phrase + 2);
  upper[strlen(upper) - 2] = '\0';
  for (char *p = upper; *p; p++)
    *p = toupper(*p);
  printf("maiusculas: %s\n", upper);

  printf("\"ro\" aparece %d vezes\n", count_occurrences(phrase, "ro"));

  char small[9];
  int want = snprintf(small, sizeof(small), "%s-%s", "o rato roeu", "a roupa do rei");
  printf("truncado: \"%s\" (pedia %d)\n", small, want);

  free(copy);
  return 0;
}
