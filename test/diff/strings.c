// Strings: strlen/strcpy/strcmp/strcat/strchr/strstr próprias, inversão,
// tokenizer com delimitadores, atoi/itoa, maiúsculas e palíndromos.
#include "kstest.h"
#include <stddef.h>

static size_t my_strlen(const char *s) {
  const char *p = s;
  while (*p) p++;
  return (size_t)(p - s);
}

static char *my_strcpy(char *d, const char *s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}

static char *my_strcat(char *d, const char *s) {
  my_strcpy(d + my_strlen(d), s);
  return d;
}

static int my_strcmp(const char *a, const char *b) {
  while (*a && *a == *b) a++, b++;
  return (unsigned char)*a - (unsigned char)*b;
}

static const char *my_strchr(const char *s, int c) {
  for (;; s++) {
    if (*s == (char)c) return s;
    if (!*s) return NULL;
  }
}

static const char *my_strstr(const char *h, const char *n) {
  if (!*n) return h;
  for (; *h; h++) {
    const char *a = h, *b = n;
    while (*a && *b && *a == *b) a++, b++;
    if (!*b) return h;
  }
  return NULL;
}

static void reverse(char *s) {
  size_t n = my_strlen(s);
  for (size_t i = 0; i < n / 2; i++) {
    char t = s[i];
    s[i] = s[n - 1 - i];
    s[n - 1 - i] = t;
  }
}

static int my_atoi(const char *s) {
  int sign = 1;
  unsigned v = 0;
  while (*s == ' ') s++;
  if (*s == '-' || *s == '+') sign = *s++ == '-' ? -1 : 1;
  while (*s >= '0' && *s <= '9') v = v * 10 + (unsigned)(*s++ - '0');
  return sign < 0 ? (int)(0u - v) : (int)v;
}

static char *my_itoa(int v, char *buf, int base) {
  char tmp[34];
  int i = 0;
  unsigned u = v < 0 && base == 10 ? 0u - (unsigned)v : (unsigned)v;
  do {
    tmp[i++] = "0123456789abcdefghijklmnopqrstuvwxyz"[u % (unsigned)base];
    u /= (unsigned)base;
  } while (u);
  char *p = buf;
  if (v < 0 && base == 10) *p++ = '-';
  while (i) *p++ = tmp[--i];
  *p = 0;
  return buf;
}

// strtok reentrante: devolve o próximo token e avança *save.
static char *tok(char **save, const char *delims) {
  char *s = *save;
  while (*s && my_strchr(delims, *s)) s++;
  if (!*s) { *save = s; return NULL; }
  char *start = s;
  while (*s && !my_strchr(delims, *s)) s++;
  if (*s) *s++ = 0;
  *save = s;
  return start;
}

static int is_palindrome(const char *s) {
  size_t i = 0, j = my_strlen(s);
  while (i < j) {
    char a = s[i], b = s[j - 1];
    if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
    if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
    if (a == ' ') { i++; continue; }
    if (b == ' ') { j--; continue; }
    if (a != b) return 0;
    i++, j--;
  }
  return 1;
}

static int sign(int x) { return (x > 0) - (x < 0); }

int main(void) {
  char buf[128];
  my_strcpy(buf, "Hello");
  my_strcat(buf, ", ");
  my_strcat(buf, "KSVM!");
  ks_put_str(buf);
  ks_put_chr('\n');
  ks_put_kv("len", (int)my_strlen(buf));
  reverse(buf);
  ks_put_str(buf);
  ks_put_chr('\n');
  const char *words[] = {"apple", "apricot", "banana", "", "app", "\xff", "Apple"};
  for (int i = 0; i < 7; i++)
    for (int j = 0; j < 7; j++) {
      ks_put_int(sign(my_strcmp(words[i], words[j])));
      ks_put_chr(j == 6 ? '\n' : ' ');
    }
  const char *hay = "the cat sat on the mat with the hat";
  for (const char *p = hay; (p = my_strstr(p, "at")) != NULL; p++) {
    ks_put_int((int)(p - hay));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("strstr(empty)", (int)(my_strstr(hay, "") - hay));
  ks_put_kv("strstr(dog)", my_strstr(hay, "dog") == NULL);
  char line[] = "  alpha,beta;;gamma  delta,,epsilon ;";
  char *save = line;
  int ntok = 0;
  for (char *t; (t = tok(&save, " ,;")) != NULL; ntok++) {
    ks_put_chr('[');
    ks_put_str(t);
    ks_put_str("] ");
  }
  ks_put_chr('\n');
  ks_put_kv("tokens", ntok);
  const char *nums[] = {"0", "-1", "+42", "  2147483647", "-2147483648", "12abc", "-0"};
  for (int i = 0; i < 7; i++) {
    int v = my_atoi(nums[i]);
    ks_put_int(v);
    ks_put_str(" ");
    ks_put_str(my_itoa(v, buf, 10));
    ks_put_str(" ");
    ks_put_str(my_itoa(v, buf, 16));
    ks_put_str(" ");
    ks_put_str(my_itoa(v, buf, 2));
    ks_put_chr('\n');
  }
  ks_put_kv("pal1", is_palindrome("A man a plan a canal Panama"));
  ks_put_kv("pal2", is_palindrome("KSVM"));
  char up[32];
  my_strcpy(up, "mixed Case 123 z{");
  for (char *p = up; *p; p++)
    if (*p >= 'a' && *p <= 'z') *p -= 'a' - 'A';
  ks_put_str(up);
  ks_put_chr('\n');
  return 0;
}
