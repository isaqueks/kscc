// kscc: "linker" por concatenação (doc 04 §3).
//
// O .ksx da ksvm2 é uma imagem final, sem relocações. Por isso não há
// objeto binário: um .kso é o texto .ksasm da unidade, e o link é a
// concatenação
//
//     crt0.ksasm + .kso do usuário (na ordem da linha de comando)
//                + libks.kso (só os blocos usados: select_used)
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
#define NEED_MARK "; kscc-need:"
#define WEAK_PREFIX "__ks_weak_"

static bool is_weak_name(char *name, int len) {
  int n = strlen(WEAK_PREFIX);
  return len > n && !strncmp(name, WEAK_PREFIX, n);
}

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

// ---------------------------------------------------------------------------
// Seleção por uso da libks
//
// libks.kso é a concatenação de todas as unidades da biblioteca. Ligar
// tudo custaria ~10 KB em todo programa (demais para o AVR) e faria as
// funções da libks colidirem com as de mesmo nome definidas pelo usuário
// (o ksas acusa label duplicado). Como num arquivo .a do Unix, só entra o
// que o programa usa:
//
//  1. libks.kso é cortada em blocos, um por label global: um label em
//     .data/.rodata/.bss, ou em .text sem '.' no nome, abre um bloco; em
//     .text, labels com '.' (L<id>.else.3, __ks_sys_ret.ok) são internos
//     à função corrente. As diretivas logo antes do label (.global,
//     .align, troca de seção) vão junto, e cada bloco começa pela sua
//     seção. O marcador "; kscc-common: nome" (definição provisória) é um
//     bloco que define `nome`.
//  2. Raízes: crt0 e os .kso do usuário entram inteiros. Todo
//     identificador citado neles (operandos de call, la, jmp, .word, ...)
//     é uma referência.
//  3. Uma referência a um nome que o usuário não define puxa o bloco da
//     libks que o define, e as referências desse bloco, até o fecho. Nome
//     definido pelo usuário vence o da libks (o common.c da suite do
//     chibicc define memcpy, strcmp, ...).
//  4. Um bloco de .text cuja última instrução não é ret/jmp/halt/trap/b
//     "cai" no bloco seguinte, que entra junto (assembly escrito à mão;
//     o codegen sempre termina funções em ret).
//  5. Referência fraca: um símbolo comum da libks cujo nome começa com
//     __ks_weak_ (definição provisória numa unidade, inicializada em
//     outra) resolve para o marcador comum, que vira um .bss zerado. A
//     definição de verdade só entra se o marcador comum foi usado (alguém
//     da libks referencia o símbolo) E uma raiz a pede com a linha
//         ; kscc-need: __ks_weak_nome
//     que o codegen emite (é comentário para o ksas). Com a definição
//     incluída, o símbolo comum some (resolve_commons). É assim que o
//     printf só liga a conversão de ponto flutuante (~20 KB) quando alguma
//     unidade passa float/double a uma função variádica, e um programa que
//     passa floats a uma variádica própria sem usar printf não a leva.
//
// Identificadores são achados por varredura léxica, ignorando comentários
// e strings; um falso positivo (um registrador com o nome de uma função,
// digamos) só inclui código a mais, nunca a menos.
// ---------------------------------------------------------------------------

typedef struct Block Block;
struct Block {
  char *name;  // símbolo definido; NULL no preâmbulo
  int name_len;
  char *text;
  size_t text_len;
  FILE *out;   // aberto enquanto o bloco é montado
  Block *next; // bloco seguinte no arquivo
  bool in_text;
  char *last_insn;  // primeira palavra da última instrução (.text)
  int last_insn_len;
  bool used;
};

typedef struct {
  Block **data;
  int len;
  int cap;
} BlockArray;

static void blocks_push(BlockArray *a, Block *b) {
  if (a->len == a->cap) {
    a->cap = a->cap ? a->cap * 2 : 64;
    a->data = realloc(a->data, sizeof(Block *) * a->cap);
  }
  a->data[a->len++] = b;
}

static bool is_ident_start(int c) {
  return isalpha(c) || c == '_' || c == '$' || c >= 0x80;
}

static bool is_ident_char(int c) {
  return is_ident_start(c) || isdigit(c) || c == '.';
}

static char *skip_space(char *p, char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\r'))
    p++;
  return p;
}

// Primeira palavra da linha (mnemônico, diretiva com o '.', ou nome).
static char *first_word(char *p, char *end, int *len) {
  p = skip_space(p, end);
  char *q = p;
  while (q < end && is_ident_char((unsigned char)*q))
    q++;
  *len = q - p;
  return p;
}

static bool word_in(char *w, int len, char **set) {
  for (int i = 0; set[i]; i++)
    if (len == (int)strlen(set[i]) && !strncmp(w, set[i], len))
      return true;
  return false;
}

static char *section_directives[] = {".text", ".data", ".rodata", ".bss", NULL};

// Diretivas que descrevem o label seguinte em vez de gerar conteúdo: vão
// para o bloco do próximo label, e seus operandos não são referências.
static char *meta_directives[] = {
  ".text", ".data", ".rodata", ".bss", ".global", ".globl", ".align",
  ".entry", ".stack", ".pic", NULL,
};

static char *terminal_insns[] = {"ret", "halt", "jmp", "j", "jmpr", "b", "trap", NULL};

typedef void RefFn(void *ctx, char *name, int len);

// Chama fn para cada identificador de [p, end) fora de comentários e
// strings. Diretivas (.word) e números não são identificadores.
static void scan_idents(char *p, char *end, RefFn *fn, void *ctx) {
  while (p < end) {
    int c = (unsigned char)*p;
    if (c == ';')
      return;
    if (c == '"' || c == '\'') {
      for (p++; p < end && *p != c; p++)
        if (*p == '\\')
          p++;
      p++;
      continue;
    }
    if (is_ident_char(c)) {
      char *q = p;
      while (q < end && is_ident_char((unsigned char)*q))
        q++;
      if (is_ident_start(c))
        fn(ctx, p, q - p);
      p = q;
      continue;
    }
    p++;
  }
}

// Chama fn para cada referência de um trecho: identificadores de todas as
// linhas, menos os labels definidos no começo da linha e as linhas de
// meta-diretivas (".global x" não usa x).
static void scan_refs(char *text, RefFn *fn, void *ctx) {
  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);

    int len;
    char *w = first_word(p, end, &len);
    if (!(w[0] == '.' && word_in(w, len, meta_directives))) {
      char *q = p;
      if (line_label(p, end, &len))
        q = strchr(p, ':') + 1;
      scan_idents(q, end, fn, ctx);
    }

    p = *end ? end + 1 : end;
  }
}

static Block *new_block(BlockArray *blocks, char *name, int len, char *section) {
  Block *b = calloc(1, sizeof(Block));
  b->name = name;
  b->name_len = len;
  b->out = open_memstream(&b->text, &b->text_len);
  if (section)
    fprintf(b->out, "\t%s\n", section);
  if (blocks->len > 0)
    blocks->data[blocks->len - 1]->next = b;
  blocks_push(blocks, b);
  return b;
}

// Linhas pendentes (comentários e meta-diretivas desde a última linha de
// conteúdo): vão para o bloco que as seguir.
typedef struct {
  FILE *out;
  char *buf;
  size_t len;
} Pending;

static void pending_open(Pending *pd) {
  pd->out = open_memstream(&pd->buf, &pd->len);
}

static void pending_flush(Pending *pd, Block *b) {
  fclose(pd->out);
  fputs(pd->buf, b->out);
  free(pd->buf);
  pending_open(pd);
}

// Bloco de um marcador de símbolo comum.
static Block *common_block(BlockArray *blocks, HashMap *defs, HashMap *weak,
                           char *line, char *end, char *section) {
  int mlen = strlen(COMMON_MARK);
  char *name = skip_space(line + mlen, end);
  char *e = name;
  while (e < end && *e != ' ' && *e != '\t')
    e++;
  if (e == name)
    error("marcador de simbolo comum invalido: %.*s", (int)(end - line), line);

  Block *b = new_block(blocks, name, e - name, section);
  hashmap_put2(is_weak_name(b->name, b->name_len) ? weak : defs, b->name, b->name_len, b);
  return b;
}

// Corta libks.kso em blocos (passo 1 acima). O primeiro bloco é o
// preâmbulo (o que vem antes do primeiro label), sempre incluído.
static void split_blocks(char *text, BlockArray *blocks, HashMap *defs, HashMap *weak) {
  char *section = ".text";
  Block *cur = new_block(blocks, NULL, 0, NULL);
  cur->used = true;

  Pending pd;
  pending_open(&pd);
  int mlen = strlen(COMMON_MARK);

  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);
    char *next = *end ? end + 1 : end;

    int wlen, llen;
    char *w = first_word(p, end, &wlen);
    char *label = line_label(p, end, &llen);
    bool in_text = !strcmp(section, ".text");

    if (!strncmp(p, COMMON_MARK, mlen)) {
      cur = common_block(blocks, defs, weak, p, end, section);
      pending_flush(&pd, cur);
      fprintf(cur->out, "%.*s\n", (int)(end - p), p);
    } else if (wlen == 0 && !label) {
      fprintf(pd.out, "%.*s\n", (int)(end - p), p);  // vazia ou comentário
    } else if (w[0] == '.' && word_in(w, wlen, meta_directives)) {
      if (word_in(w, wlen, section_directives))
        section = strndup(w, wlen);
      fprintf(pd.out, "%.*s\n", (int)(end - p), p);
    } else {
      if (label && !(in_text && memchr(label, '.', llen))) {
        cur = new_block(blocks, label, llen, section);
        cur->in_text = in_text;
        hashmap_put2(defs, label, llen, cur);
      }
      pending_flush(&pd, cur);
      fprintf(cur->out, "%.*s\n", (int)(end - p), p);

      // Instrução da linha (depois do label, se houver).
      char *body = label ? strchr(p, ':') + 1 : p;
      int blen;
      char *bw = first_word(body, end, &blen);
      if (in_text && blen > 0 && bw[0] != '.') {
        cur->last_insn = bw;
        cur->last_insn_len = blen;
      }
    }
    p = next;
  }

  pending_flush(&pd, cur);
  fclose(pd.out);
  free(pd.buf);
  for (int i = 0; i < blocks->len; i++)
    fclose(blocks->data[i]->out);
}

static bool falls_through(Block *b) {
  return b->in_text &&
         (!b->last_insn || !word_in(b->last_insn, b->last_insn_len, terminal_insns));
}

typedef struct {
  HashMap user_defs;  // labels e símbolos comuns definidos pelas raízes
  HashMap lib_defs;   // nome -> Block da libks
  HashMap lib_weak;   // nome -> marcador comum __ks_weak_* da libks (passo 5)
  StringArray needs;  // nomes pedidos por "; kscc-need:" nas raízes
  BlockArray work;    // blocos marcados cujas referências faltam seguir
} GcContext;

static void mark_block(GcContext *gc, Block *b) {
  if (b->used)
    return;
  b->used = true;
  blocks_push(&gc->work, b);
}

static void use_name(void *ctx, char *name, int len) {
  GcContext *gc = ctx;
  if (hashmap_get2(&gc->user_defs, name, len))
    return;
  Block *b = NULL;
  if (is_weak_name(name, len))
    b = hashmap_get2(&gc->lib_weak, name, len);
  if (!b)
    b = hashmap_get2(&gc->lib_defs, name, len);
  if (b)
    mark_block(gc, b);
}

// Linhas "; kscc-need: nome" de uma raiz: a definição de verdade de um
// símbolo fraco é pedida (passo 5; atendida em resolve_needs).
static void collect_needs(GcContext *gc, char *text) {
  int mlen = strlen(NEED_MARK);
  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);

    if (!strncmp(p, NEED_MARK, mlen)) {
      char *q = skip_space(p + mlen, end);
      char *e = q;
      while (e < end && is_ident_char((unsigned char)*e))
        e++;
      if (e > q && !hashmap_get2(&gc->user_defs, q, e - q))
        strarray_push(&gc->needs, strndup(q, e - q));
    }

    p = *end ? end + 1 : end;
  }
}

// Segue as referências dos blocos marcados até o fecho.
static void gc_close(GcContext *gc) {
  while (gc->work.len > 0) {
    Block *b = gc->work.data[--gc->work.len];
    scan_refs(b->text, use_name, gc);
    if (falls_through(b) && b->next)
      mark_block(gc, b->next);
  }
}

// Atende os pedidos de definição de símbolos fracos cujo marcador comum
// entrou (passo 5). Devolve true se marcou algum bloco.
static bool resolve_needs(GcContext *gc) {
  bool changed = false;
  for (int i = 0; i < gc->needs.len; i++) {
    char *name = gc->needs.data[i];
    Block *weak = hashmap_get(&gc->lib_weak, name);
    Block *def = hashmap_get(&gc->lib_defs, name);
    if (weak && weak->used && def && !def->used) {
      mark_block(gc, def);
      changed = true;
    }
  }
  return changed;
}

static void collect_user_defs(char *text, HashMap *defs) {
  int mlen = strlen(COMMON_MARK);
  for (char *p = text; *p;) {
    char *end = strchr(p, '\n');
    if (!end)
      end = p + strlen(p);

    int len;
    char *name = line_label(p, end, &len);
    if (name)
      hashmap_put2(defs, name, len, (void *)1);
    if (!strncmp(p, COMMON_MARK, mlen)) {
      char *q = skip_space(p + mlen, end);
      char *e = q;
      while (e < end && *e != ' ' && *e != '\t')
        e++;
      hashmap_put2(defs, q, e - q, (void *)1);
    }

    p = *end ? end + 1 : end;
  }
}

// Devolve só os blocos de `lib` que as raízes usam, na ordem original.
static char *select_used(char *lib, char **roots, int nroots) {
  GcContext gc = {0};
  BlockArray blocks = {0};
  split_blocks(lib, &blocks, &gc.lib_defs, &gc.lib_weak);

  for (int i = 0; i < nroots; i++)
    collect_user_defs(roots[i], &gc.user_defs);
  for (int i = 0; i < nroots; i++) {
    collect_needs(&gc, roots[i]);
    scan_refs(roots[i], use_name, &gc);
  }

  do
    gc_close(&gc);
  while (resolve_needs(&gc));

  char *buf;
  size_t buflen;
  FILE *out = open_memstream(&buf, &buflen);
  for (int i = 0; i < blocks.len; i++)
    if (blocks.data[i]->used)
      fputs(blocks.data[i]->text, out);
  fprintf(out, "\t.text\n");
  fclose(out);
  return buf;
}

void ks_link(LinkOptions *o, StringArray *inputs, StringArray *names, char *output) {
  char *crt0 = format("%s/crt0.ksasm", o->libdir);
  char *libks = format("%s/libks.kso", o->libdir);

  if (driver_dry_run()) {
    fprintf(stderr, "kscc-link");
    if (!o->nostdlib)
      fprintf(stderr, " %s", crt0);
    for (int i = 0; i < inputs->len; i++)
      fprintf(stderr, " %s", inputs->data[i]);
    if (!o->nostdlib)
      fprintf(stderr, " %s%s", libks, o->no_gc ? "" : " (so o usado)");
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
    if (!file_exists(crt0) || !file_exists(libks))
      error("libks nao encontrada em %s (rode make, defina KSCC_LIBDIR ou use -nostdlib)",
            o->libdir);
    tnames[ntexts] = crt0;
    texts[ntexts++] = must_slurp(crt0, crt0);
  }

  for (int i = 0; i < inputs->len; i++) {
    tnames[ntexts] = names->data[i];
    texts[ntexts++] = must_slurp(inputs->data[i], names->data[i]);
  }

  if (!o->nostdlib) {
    char *lib = must_slurp(libks, libks);
    if (!o->no_gc) {
      lib = select_used(lib, texts, ntexts);
      libks = format("%s (blocos usados)", libks);
    }
    tnames[ntexts] = libks;
    texts[ntexts++] = lib;
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
