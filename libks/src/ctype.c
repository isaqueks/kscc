// <ctype.h> da libks (doc 05 §2): uma tabela de 128 bytes em .rodata com
// um bit por classe; cada função é um teste de bit. Fora de 0..127
// (inclusive EOF e bytes > 127 vindos de char com sinal) nada pertence a
// classe alguma.

#define C_DIGIT 0x01
#define C_UPPER 0x02
#define C_LOWER 0x04
#define C_SPACE 0x08
#define C_XDIGIT 0x10
#define C_PRINT 0x20

#define C_ 0                       // controle
#define CS C_SPACE                 // \t \n \v \f \r
#define PS (C_PRINT | C_SPACE)     // ' '
#define P_ C_PRINT                 // pontuação
#define PD (C_PRINT | C_DIGIT | C_XDIGIT)
#define PU (C_PRINT | C_UPPER)
#define PX (C_PRINT | C_UPPER | C_XDIGIT)
#define PL (C_PRINT | C_LOWER)
#define PY (C_PRINT | C_LOWER | C_XDIGIT)

static const unsigned char table[128] = {
  C_, C_, C_, C_, C_, C_, C_, C_, C_, CS, CS, CS, CS, CS, C_, C_,  // 0x00
  C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_, C_,  // 0x10
  PS, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_,  // 0x20  !"#...
  PD, PD, PD, PD, PD, PD, PD, PD, PD, PD, P_, P_, P_, P_, P_, P_,  // 0x30 0-9
  P_, PX, PX, PX, PX, PX, PX, PU, PU, PU, PU, PU, PU, PU, PU, PU,  // 0x40 @A-O
  PU, PU, PU, PU, PU, PU, PU, PU, PU, PU, PU, P_, P_, P_, P_, P_,  // 0x50 P-Z
  P_, PY, PY, PY, PY, PY, PY, PL, PL, PL, PL, PL, PL, PL, PL, PL,  // 0x60 `a-o
  PL, PL, PL, PL, PL, PL, PL, PL, PL, PL, PL, P_, P_, P_, P_, C_,  // 0x70 p-z, DEL
};

static int is(int c, int mask) {
  return (unsigned)c < 128 && (table[c] & mask) != 0;
}

int isdigit(int c) { return is(c, C_DIGIT); }
int isalpha(int c) { return is(c, C_UPPER | C_LOWER); }
int isalnum(int c) { return is(c, C_UPPER | C_LOWER | C_DIGIT); }
int isspace(int c) { return is(c, C_SPACE); }
int isupper(int c) { return is(c, C_UPPER); }
int islower(int c) { return is(c, C_LOWER); }
int isxdigit(int c) { return is(c, C_XDIGIT); }
int isprint(int c) { return is(c, C_PRINT); }

int toupper(int c) { return islower(c) ? c - 'a' + 'A' : c; }
int tolower(int c) { return isupper(c) ? c - 'A' + 'a' : c; }
