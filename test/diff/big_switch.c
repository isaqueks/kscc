// switch grande: casos densos e esparsos, negativos, fallthrough, default no
// meio, switch aninhado, switch sobre char e unsigned, break/continue em laço.
#include "kstest.h"

static int dense(int x) {
  switch (x) {
  case 0: return 100;
  case 1: return 101;
  case 2: return 104;
  case 3: return 109;
  case 4: return 116;
  case 5: return 125;
  case 6: return 136;
  case 7: return 149;
  case 8: return 164;
  case 9: return 181;
  case 10: return 200;
  case 11: return 221;
  case 12: return 244;
  case 13: return 269;
  case 14: return 296;
  case 15: return 325;
  case 16: return 356;
  case 17: return 389;
  case 18: return 424;
  case 19: return 461;
  case 20: return 500;
  case 21: return 541;
  case 22: return 584;
  case 23: return 629;
  case 24: return 676;
  case 25: return 725;
  case 26: return 776;
  case 27: return 829;
  case 28: return 884;
  case 29: return 941;
  case 30: return 1000;
  case 31: return 1061;
  default: return -1;
  }
}

static int sparse(int x) {
  int r = 0;
  switch (x) {
  case -2147483647 - 1: r = 1; break;
  case -1000000: r = 2; break;
  case -7: r = 3; break;
  case -1: r = 4; break;
  default: r = 99; break;
  case 0: r = 5; break;
  case 64: r = 6; break;
  case 1 << 20: r = 7; break;
  case 0x7FFFFFFF: r = 8; break;
  case 12345: r = 9;
  }
  return r;
}

static int fall(int x) {
  int acc = 0;
  switch (x) {
  case 5: acc += 5; // fallthrough
  case 4: acc += 4; // fallthrough
  case 3: acc += 3; break;
  case 2: acc += 2; // fallthrough
  default: acc += 1000; // fallthrough
  case 1: acc += 1;
  }
  return acc;
}

static const char *kind(char c) {
  switch (c) {
  case 'a': case 'e': case 'i': case 'o': case 'u':
    return "vogal";
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    return "digito";
  case ' ': case '\t': case '\n':
    return "espaco";
  case (char)0xE9:
    return "e-agudo";
  default:
    if (c >= 'a' && c <= 'z') return "consoante";
    return "outro";
  }
}

static unsigned uswitch(unsigned x) {
  switch (x) {
  case 0xFFFFFFFFu: return 1;
  case 0x80000000u: return 2;
  case 3000000000u: return 3;
  case 1u: return 4;
  }
  return 0;
}

static int nested(int a, int b) {
  switch (a) {
  case 0:
    switch (b) {
    case 0: return 0;
    case 1: return 1;
    default: break;
    }
    return 2;
  case 1:
    switch (b) {
    case 0: a = 10; break;
    case 1: a = 11; // fallthrough
    default: a += 100;
    }
    break;
  default:
    return -a * 10 - b;
  }
  return a;
}

// switch com escopo e declarações dentro; laço com continue a partir do switch
static int loop_switch(void) {
  int total = 0;
  for (int i = 0; i < 20; i++) {
    switch (i % 5) {
    case 0:
      continue;
    case 1: {
      int k = i * 2;
      total += k;
      break;
    }
    case 2:
      if (i > 10) break;
      total += 1000;
      break;
    default:
      total -= i;
    }
    total += 1;
  }
  return total;
}

// Duff's device
static void duff_copy(char *to, const char *from, int count) {
  int n = (count + 7) / 8;
  switch (count % 8) {
  case 0: do { *to++ = *from++;
  case 7:      *to++ = *from++;
  case 6:      *to++ = *from++;
  case 5:      *to++ = *from++;
  case 4:      *to++ = *from++;
  case 3:      *to++ = *from++;
  case 2:      *to++ = *from++;
  case 1:      *to++ = *from++;
          } while (--n > 0);
  }
}

int main(void) {
  unsigned s = 0;
  for (int i = -3; i < 36; i++)
    s = s * 31u + (unsigned)dense(i);
  ks_put_kx("dense", s);
  int probes[] = {-2147483647 - 1, -1000000, -999999, -7, -1, 0, 1, 64, 65, 1 << 20, 0x7FFFFFFF, 12345};
  for (int i = 0; i < 12; i++) {
    ks_put_int(sparse(probes[i]));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  for (int i = 0; i <= 6; i++) {
    ks_put_int(fall(i));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  const char *txt = "ok 42\tz\xe9!";
  for (const char *p = txt; *p; p++) {
    ks_put_str(kind(*p));
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("uswitch", (int)(uswitch(0xFFFFFFFFu) * 1000 + uswitch(0x80000000u) * 100 +
                             uswitch(3000000000u) * 10 + uswitch(1) + uswitch(2)));
  for (int a = 0; a < 3; a++)
    for (int b = 0; b < 3; b++) {
      ks_put_int(nested(a, b));
      ks_put_chr(' ');
    }
  ks_put_chr('\n');
  ks_put_kv("loop_switch", loop_switch());
  char dst[40] = {0};
  const char *src = "Duff's device copies in chunks!";
  for (int c = 0; c <= 31; c += 3) {
    for (int i = 0; i < 40; i++) dst[i] = 0;
    duff_copy(dst, src, c ? c : 1);
    ks_put_str(dst);
    ks_put_chr('\n');
  }
  return 0;
}
