// kscc: "linker" por concatenação (doc 04 §3).
//
// O .ksx da ksvm2 é uma imagem final, sem relocações. Por isso não há
// objeto binário: um .kso é o texto .ksasm da unidade, e o link é a
// concatenação
//
//     crt0.ksasm + .kso do usuário (na ordem da linha de comando)
//                + runtime (lib/libks.kso; até a fase 4, lib/minirt.ksasm)
//                + símbolos comuns sem definição (ver resolve_commons)
//                + rodapé (.entry _start / .stack N)
//
// em um único .ksasm montado pelo ksas. Os erros do ksas, que se referem
// ao arquivo concatenado, são remapeados para o arquivo de origem de cada
// trecho.
//
// Este arquivo é novo no kscc (não existe no chibicc).

#include "chibicc.h"

typedef struct {
  char *name;     // nome de exibição (fonte .c, .kso ou .ksasm)
  int first_line; // primeira linha do trecho no arquivo concatenado
  int nlines;
} Part;

typedef struct {
  Part *data;
  int len;
  int cap;
} PartArray;

static void parts_push(PartArray *a, Part p) {
  if (a->len == a->cap) {
    a->cap = a->cap ? a->cap * 2 : 8;
    a->data = realloc(a->data, sizeof(Part) * a->cap);
  }
  a->data[a->len++] = p;
}

// Lê um arquivo inteiro. Devolve NULL se não for possível abrir.
static char *slurp(char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp)
    return NULL;

  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);
  for (;;) {
    char tmp[4096];
    size_t n = fread(tmp, 1, sizeof(tmp), fp);
    if (n == 0)
      break;
    fwrite(tmp, 1, n, out);
  }
  fclose(fp);
  fclose(out);
  return buf;
}

static char *must_slurp(char *path, char *display) {
  char *s = slurp(path);
  if (!s)
    error("%s: %s", display, strerror(errno));
  return s;
}

static int count_lines(char *s) {
  int n = 0;
  for (; *s; s++)
    if (*s == '\n')
      n++;
  return n;
}

void ks_copy_file(char *src, char *dst) {
  if (driver_dry_run()) {
    fprintf(stderr, "cp %s %s\n", src, dst);
    return;
  }

  char *text = must_slurp(src, src);
  FILE *out = fopen(dst, "w");
  if (!out)
    error("nao foi possivel abrir o arquivo de saida: %s: %s", dst, strerror(errno));
  fputs(text, out);
  fclose(out);
  free(text);
}

// Acrescenta um trecho ao arquivo concatenado, garantindo '\n' no fim.
static void add_part(FILE *out, PartArray *parts, int *line, char *name, char *text) {
  fprintf(out, "; ---- kscc link: %s\n", name);
  (*line)++;

  int n = count_lines(text);
  fputs(text, out);
  if (*text && text[strlen(text) - 1] != '\n') {
    fputc('\n', out);
    n++;
  }

  parts_push(parts, (Part){name, *line, n});
  *line += n;
}

// Extrai X de "... simbolo 'X' nao definido". Devolve NULL se a linha não
// é desse tipo.
static char *undefined_symbol(char *line) {
  char *p = strstr(line, "nao definido");
  if (!p)
    return NULL;
  char *q1 = strchr(line, '\'');
  if (!q1 || q1 > p)
    return NULL;
  char *q2 = strchr(q1 + 1, '\'');
  if (!q2 || q2 > p)
    return NULL;
  return strndup(q1 + 1, q2 - q1 - 1);
}

static bool strarray_has(StringArray *a, char *s) {
  for (int i = 0; i < a->len; i++)
    if (!strcmp(a->data[i], s))
      return true;
  return false;
}

// Imprime as mensagens do ksas trocando "tmp:linha:col" pelo arquivo e
// linha de origem. Devolve em `undef` os símbolos indefinidos citados.
static void report_errors(char *errfile, char *asmfile, PartArray *parts,
                          StringArray *undef, bool print) {
  char *text = slurp(errfile);
  if (!text)
    return;

  int alen = strlen(asmfile);

  for (char *line = strtok(text, "\n"); line; line = strtok(NULL, "\n")) {
    char *sym = undefined_symbol(line);
    if (sym && !strarray_has(undef, sym))
      strarray_push(undef, sym);

    if (!print)
      continue;

    if (!strncmp(line, asmfile, alen) && line[alen] == ':') {
      char *end;
      long ln = strtol(line + alen + 1, &end, 10);
      for (int i = 0; i < parts->len; i++) {
        Part *p = &parts->data[i];
        if (p->first_line <= ln && ln < p->first_line + p->nlines) {
          fprintf(stderr, "%s:%ld%s\n", p->name, ln - p->first_line + 1, end);
          goto next;
        }
      }
    }
    fprintf(stderr, "%s\n", line);
  next:;
  }
  free(text);
}

static int run_ksas(LinkOptions *o, char *input, char *output, char *errfile) {
  char *argv[] = {o->ksas, input, "-o", output, o->debug ? "-g" : NULL, NULL};
  return run_subprocess_capture(argv, errfile);
}

// Valida uma unidade sozinha (kscc -c). Os símbolos externos ainda não
// existem, então uma primeira passada do ksas descobre quais são; se esses
// forem os únicos erros, uma segunda passada com labels-esboço para eles
// confirma que o resto da unidade monta. O .kso gravado não contém os
// esboços.
// Arquivo de validação: a unidade + uma instrução-esboço (o ksas recusa
// imagem sem nenhuma instrução em .text, caso de uma unidade só com dados)
// + labels-esboço para os símbolos externos. Os esboços ficam antes da
// instrução: um label depois da última instrução aponta para fora do
// código, e o ksas recusa desvio para lá (o caso do crt0, cujo .text
// termina logo depois de "call exit").
static char *write_check_file(char *text, StringArray *undef) {
  char *tmp = create_tmpfile();
  FILE *out = fopen(tmp, "w");
  if (!out)
    error("%s: %s", tmp, strerror(errno));
  fputs(text, out);
  fprintf(out, "\n; kscc: esbocos para validar a unidade isolada\n\t.text\n");
  fprintf(out, "__kscc_check_stub:\n");
  for (int i = 0; i < undef->len; i++)
    fprintf(out, "%s:\n", undef->data[i]);
  fprintf(out, "\thalt\n");
  fclose(out);
  return tmp;
}

void ks_check_unit(LinkOptions *o, char *path, char *display) {
  if (driver_dry_run()) {
    char *argv[] = {o->ksas, path, "-o", "/dev/null", NULL};
    run_subprocess_capture(argv, NULL);
    return;
  }

  char *text = must_slurp(path, display);
  PartArray parts = {0};
  parts_push(&parts, (Part){display, 1, count_lines(text) + 1});

  StringArray undef = {0};
  char *tmp = write_check_file(text, &undef);
  char *errfile = create_tmpfile();
  if (run_ksas(o, tmp, "/dev/null", errfile) == 0)
    return;

  // Só símbolos indefinidos? Conta as linhas de erro que não são isso.
  char *errs = slurp(errfile);
  bool other = false;
  for (char *line = strtok(errs, "\n"); line; line = strtok(NULL, "\n"))
    if (strstr(line, ": erro:") && !undefined_symbol(line))
      other = true;
  free(errs);

  report_errors(errfile, tmp, &parts, &undef, other);
  if (other || undef.len == 0) {
    if (!other)
      report_errors(errfile, tmp, &parts, &undef, true);
    exit(1);
  }

  tmp = write_check_file(text, &undef);
  if (run_ksas(o, tmp, "/dev/null", errfile) != 0) {
    StringArray undef2 = {0};
    report_errors(errfile, tmp, &parts, &undef2, true);
    exit(1);
  }
}

// Símbolos comuns (definições provisórias, `int x;`). O ksas não tem
// .comm, então o codegen emite em cada unidade uma linha
//
//     ; kscc-common: nome tamanho alinhamento
//
// que o ksas ignora (é comentário). Aqui juntamos as de todos os trechos:
// se algum trecho define o label `nome:` (uma definição com
// inicializador, ou assembly escrito à mão), o símbolo comum some, como
// no linker do Unix; senão é criado um único .bss com o maior tamanho e o
// maior alinhamento pedidos.
typedef struct Common Common;
struct Common {
  Common *next;
  char *name;
  int size;
  int align;
};

#define COMMON_MARK "; kscc-common:"

// Nome do label definido na linha (`nome:` no começo, depois de
// espaços), ou NULL.
static char *line_label(char *p, char *end, int *len) {
  while (p < end && (*p == ' ' || *p == '\t'))
    p++;
  char *q = p;
  while (q < end && (isalnum((unsigned char)*q) || *q == '_' || *q == '.' || *q == '$'))
    q++;
  if (q == p || q >= end || *q != ':')
    return NULL;
  *len = q - p;
  return p;
}

static void scan_commons(char *text, Common **list, HashMap *commons) {
  int mlen = strlen(COMMON_MARK);
  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);

    if (!strncmp(p, COMMON_MARK, mlen)) {
      char name[512];
      int size, align;
      if (sscanf(p + mlen, "%511s %d %d", name, &size, &align) != 3)
        error("marcador de simbolo comum invalido: %.*s", (int)(end - p), p);

      Common *c = hashmap_get(commons, name);
      if (!c) {
        c = calloc(1, sizeof(Common));
        c->name = strdup(name);
        c->next = *list;
        *list = c;
        hashmap_put(commons, c->name, c);
      }
      c->size = MAX(c->size, size);
      c->align = MAX(c->align, align);
    }

    p = *end ? end + 1 : end;
  }
}

static void scan_definitions(char *text, HashMap *commons, HashMap *defined) {
  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);

    int len;
    char *name = line_label(p, end, &len);
    if (name && hashmap_get2(commons, name, len))
      hashmap_put2(defined, name, len, (void *)1);

    p = *end ? end + 1 : end;
  }
}

// Devolve o texto .ksasm com as definições dos símbolos comuns que nenhum
// trecho define (ou "" se não há).
static char *resolve_commons(char **texts, int ntexts) {
  Common *list = NULL;
  HashMap commons = {0};
  for (int i = 0; i < ntexts; i++)
    scan_commons(texts[i], &list, &commons);
  if (!list)
    return "";

  HashMap defined = {0};
  for (int i = 0; i < ntexts; i++)
    scan_definitions(texts[i], &commons, &defined);

  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);
  for (Common *c = list; c; c = c->next) {
    if (hashmap_get(&defined, c->name))
      continue;
    fprintf(out, "\t.bss\n");
    if (c->align > 1)
      fprintf(out, "\t.align\t%d\n", c->align);
    fprintf(out, "\t.global\t%s\n%s:\n", c->name, c->name);
    if (c->size > 0)
      fprintf(out, "\t.space\t%d\n", c->size);
  }
  fprintf(out, "\t.text\n");
  fclose(out);
  return buf;
}

void ks_link(LinkOptions *o, StringArray *inputs, StringArray *names, char *output) {
  char *crt0 = format("%s/lib/crt0.ksasm", o->prefix);
  char *libks = format("%s/lib/libks.kso", o->prefix);
  char *minirt = format("%s/lib/minirt.ksasm", o->prefix);
  char *runtime = file_exists(libks) ? libks : minirt;

  if (driver_dry_run()) {
    fprintf(stderr, "kscc-link");
    if (!o->nostdlib)
      fprintf(stderr, " %s", crt0);
    for (int i = 0; i < inputs->len; i++)
      fprintf(stderr, " %s", inputs->data[i]);
    if (!o->nostdlib)
      fprintf(stderr, " %s", runtime);
    fprintf(stderr, " --stack=%d -o <tmp>.ksasm\n", o->stack_size);
    run_ksas(o, "<tmp>.ksasm", output, NULL);
    return;
  }

  char *asmfile = create_tmpfile();
  FILE *out = fopen(asmfile, "w");
  if (!out)
    error("%s: %s", asmfile, strerror(errno));

  PartArray parts = {0};
  int line = 1;

  // Trechos na ordem da concatenação.
  int ntexts = 0;
  char **texts = calloc(inputs->len + 2, sizeof(char *));
  char **tnames = calloc(inputs->len + 2, sizeof(char *));

  if (!o->nostdlib) {
    if (!file_exists(crt0))
      error("crt0 nao encontrado: %s (use KSCC_PREFIX ou -nostdlib)", crt0);
    tnames[ntexts] = crt0;
    texts[ntexts++] = must_slurp(crt0, crt0);
  }

  for (int i = 0; i < inputs->len; i++) {
    tnames[ntexts] = names->data[i];
    texts[ntexts++] = must_slurp(inputs->data[i], names->data[i]);
  }

  if (!o->nostdlib) {
    if (!file_exists(runtime))
      error("runtime nao encontrado: %s", runtime);
    tnames[ntexts] = runtime;
    texts[ntexts++] = must_slurp(runtime, runtime);
  }

  fprintf(out, "; kscc: programa ligado (concatenacao, doc 04 §3)\n");
  line++;

  for (int i = 0; i < ntexts; i++)
    add_part(out, &parts, &line, tnames[i], texts[i]);

  char *commons = resolve_commons(texts, ntexts);
  if (*commons)
    add_part(out, &parts, &line, "<simbolos comuns>", commons);

  add_part(out, &parts, &line, "<rodape do link>",
           format("\t.entry\t_start\n\t.stack\t%d\n", o->stack_size));
  fclose(out);

  char *errfile = create_tmpfile();
  if (run_ksas(o, asmfile, output, errfile) == 0)
    return;

  StringArray undef = {0};
  report_errors(errfile, asmfile, &parts, &undef, true);
  for (int i = 0; i < undef.len; i++)
    fprintf(stderr, "kscc: referencia indefinida a '%s'\n", undef.data[i]);
  exit(1);
}
