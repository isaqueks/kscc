// Arrays multidimensionais: 3D, ponteiro para array, array de ponteiros
// para linhas, sizeof das dimensões, jogo da vida num tabuleiro toroidal e
// triângulo de Pascal.
#include "kstest.h"

#define R 10
#define C 12

static unsigned char board[2][R][C];

static int neighbors(unsigned char (*b)[C], int r, int c) {
  int n = 0;
  for (int dr = -1; dr <= 1; dr++)
    for (int dc = -1; dc <= 1; dc++)
      if (dr || dc)
        n += b[(r + dr + R) % R][(c + dc + C) % C];
  return n;
}

static void step(int from) {
  for (int r = 0; r < R; r++)
    for (int c = 0; c < C; c++) {
      int n = neighbors(board[from], r, c);
      board[!from][r][c] = n == 3 || (n == 2 && board[from][r][c]);
    }
}

static void show(unsigned char (*b)[C]) {
  for (int r = 0; r < R; r++) {
    for (int c = 0; c < C; c++)
      ks_put_chr(b[r][c] ? '#' : '.');
    ks_put_chr('\n');
  }
}

static int sum3(int (*cube)[3][4], int n) {
  int s = 0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 4; k++)
        s += cube[i][j][k] * (i + 1);
  return s;
}

int main(void) {
  // glider + blinker
  board[0][1][2] = board[0][2][3] = board[0][3][1] = board[0][3][2] = board[0][3][3] = 1;
  board[0][7][7] = board[0][7][8] = board[0][7][9] = 1;
  int cur = 0;
  for (int g = 0; g < 9; g++) {
    step(cur);
    cur = !cur;
  }
  show(board[cur]);
  int pop = 0;
  for (int r = 0; r < R; r++)
    for (int c = 0; c < C; c++)
      pop += board[cur][r][c];
  ks_put_kv("pop", pop);

  int cube[2][3][4];
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 4; k++)
        cube[i][j][k] = i * 100 + j * 10 + k;
  ks_put_kv("sum3", sum3(cube, 2));
  ks_put_kv("sizeof cube", (int)sizeof cube);
  ks_put_kv("sizeof cube[1]", (int)sizeof cube[1]);
  ks_put_kv("sizeof cube[1][2]", (int)sizeof cube[1][2]);
  ks_put_kv("&cube[1][2][3] - &cube[0][0][0]", (int)(&cube[1][2][3] - &cube[0][0][0]));
  int (*row)[4] = cube[1];
  ks_put_kv("row[2][3]", row[2][3]);
  ks_put_kv("*(*(row + 1) + 2)", *(*(row + 1) + 2));
  int *flat = &cube[0][0][0];
  ks_put_kv("flat[23]", flat[23]);

  // triângulo de Pascal num array 2D e linhas por ponteiros
  static unsigned pas[20][20];
  for (int n = 0; n < 20; n++) {
    pas[n][0] = pas[n][n] = 1;
    for (int k = 1; k < n; k++)
      pas[n][k] = pas[n - 1][k - 1] + pas[n - 1][k];
  }
  unsigned *rows[20];
  for (int n = 0; n < 20; n++)
    rows[n] = pas[n];
  for (int k = 0; k <= 8; k++) {
    ks_put_uint(rows[8][k]);
    ks_put_chr(' ');
  }
  ks_put_chr('\n');
  ks_put_kv("C(19,9)", (int)rows[19][9]);
  // strings 2D
  char names[4][8] = {"zero", "um", "dois", "tres"};
  for (int n = 3; n >= 0; n--) {
    ks_put_str(names[n]);
    ks_put_chr(n ? ',' : '\n');
  }
  ks_put_kv("names[2][3]", names[2][3]);
  return 0;
}
