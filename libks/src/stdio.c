// <stdio.h> da libks, entrada e saída de caracteres (doc 05 §2). A
// formatação (printf e família) está em printf.c.
//
// stdout tem um buffer de 128 bytes descarregado em '\n', quando enche,
// em fflush, em exit (por __ks_stdio_flush, ver exit.c) e antes de ler
// stdin (para o prompt aparecer). stderr não tem buffer, mas descarrega
// stdout antes de escrever, para as duas saídas, que vão ao mesmo
// console, ficarem na ordem em que o programa as produziu.
#include <errno.h>
#include <ksvm/syscall.h>
#include <stdio.h>

extern void (*__ks_stdio_flush)(void);

static char out_buf[BUFSIZ];
static unsigned out_len;

static void flush_stdout(void) {
  unsigned done = 0;

  // PUTS pode aceitar menos bytes que o pedido; erro descarta o resto.
  while (done < out_len) {
    int n = ks_sys_puts(out_buf + done, out_len - done);
    if (n <= 0)
      break;
    done += n;
  }
  out_len = 0;
}

// Um byte para stdout (também usado por printf.c).
void __ks_stdout_putc(int c) {
  if (out_len == 0)
    __ks_stdio_flush = flush_stdout;
  out_buf[out_len++] = c;
  if (c == '\n' || out_len == BUFSIZ)
    flush_stdout();
}

// Um byte para stderr (também usado por printf.c, que descarrega stdout
// uma vez por chamada antes de usar esta).
void __ks_stderr_putc(int c) {
  ks_sys_putc(c);
}

int fflush(FILE *f) {
  if (f == NULL || f == stdout)
    flush_stdout();
  return 0;
}

int fputc(int c, FILE *f) {
  if (f == stdout) {
    __ks_stdout_putc(c);
  } else if (f == stderr) {
    flush_stdout();
    if (ks_sys_putc(c) < 0)
      return EOF;
  } else {
    return EOF;
  }
  return (unsigned char)c;
}

int putc(int c, FILE *f) { return fputc(c, f); }
int putchar(int c) { return fputc(c, stdout); }

int fputs(const char *restrict s, FILE *restrict f) {
  if (f == stderr) {
    unsigned n = 0;
    while (s[n])
      n++;
    flush_stdout();
    return ks_sys_puts(s, n) < 0 ? EOF : 0;
  }
  for (; *s; s++)
    if (fputc(*s, f) == EOF)
      return EOF;
  return 0;
}

int puts(const char *s) {
  if (fputs(s, stdout) == EOF)
    return EOF;
  return putchar('\n') == EOF ? EOF : 0;
}

// GETC não bloqueia: sem dado ainda, a VM devolve -1 com EAGAIN. Aqui
// esperamos cedendo 1 ms por vez (no MCU, em modo de baixo consumo). EOF
// de verdade é KSVM_ERR_EOF, ou ENOSYS se a HAL não tem console. errno é
// restaurado depois das tentativas que só deram EAGAIN.
int getchar(void) {
  int saved = errno;

  flush_stdout();
  for (;;) {
    int c = ks_sys_getc();
    if (c >= 0) {
      errno = saved;
      return c;
    }
    if (errno != EAGAIN)
      return EOF;
    ks_sys_sleep_ms(1);
  }
}

int fgetc(FILE *f) { return f == stdin ? getchar() : EOF; }
int getc(FILE *f) { return fgetc(f); }

// Como o C: até n-1 bytes ou '\n' (incluso), com '\0' no fim; NULL se a
// entrada acabou antes de qualquer byte.
char *fgets(char *restrict buf, int n, FILE *restrict f) {
  int i = 0;

  if (n <= 0)
    return NULL;
  while (i < n - 1) {
    int c = fgetc(f);
    if (c == EOF) {
      if (i == 0)
        return NULL;
      break;
    }
    buf[i++] = c;
    if (c == '\n')
      break;
  }
  buf[i] = '\0';
  return buf;
}

// Lê uma linha sem o '\n'. Diferente do C11 (que trata linha longa demais
// como violação de restrição e devolve NULL), aqui a linha é truncada em
// n-1 bytes e o resto dela descartado: num console serial é mais útil
// que perder a linha inteira.
char *gets_s(char *buf, size_t n) {
  size_t i = 0;
  int c, any = 0;

  if (buf == NULL || n == 0)
    return NULL;
  while ((c = getchar()) != EOF && c != '\n') {
    any = 1;
    if (i < n - 1)
      buf[i++] = c;
  }
  buf[i] = '\0';
  return (c == EOF && !any) ? NULL : buf;
}
