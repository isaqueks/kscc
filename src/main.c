// kscc: driver. Derivado do main.c do chibicc (Rui Ueyama, MIT).
//
// Diferenças em relação ao chibicc (doc 04):
//  - "assemble" valida a unidade com o ksas e grava o .kso, que é o próprio
//    texto .ksasm (não há formato de objeto binário);
//  - "link" é a concatenação crt0 + .kso + libks (só o que é usado) +
//    rodapé, montada pelo ksas em um .ksx (src/link.c);
//  - caminhos de include: só -I, <prefixo>/libks/include e
//    <prefixo>/include, nunca /usr/include;
//  - opções de linker do host (-static, -shared, -L, -l, -Xlinker) saem.

#include "chibicc.h"

typedef enum {
  FILE_NONE, FILE_C, FILE_ASM, FILE_OBJ,
} FileType;

StringArray include_paths;
// kscc: como no chibicc, definições provisórias são símbolos comuns por
// padrão. O ksas não tem .comm: o codegen emite um marcador e o link
// (src/link.c) faz a deduplicação. -fno-common as torna .bss comuns.
bool opt_fcommon = true;
bool opt_fpic;

// Identificador curto e único da unidade de tradução (doc 02 §5).
char *unit_id;

static FileType opt_x;
static StringArray opt_include;
static bool opt_E;
static bool opt_M;
static bool opt_MD;
static bool opt_MMD;
static bool opt_MP;
static bool opt_S;
static bool opt_c;
static bool opt_cc1;
static bool opt_hash_hash_hash;
static bool opt_nostdlib;
static bool opt_no_gc;
static bool opt_g;
static char *opt_MF;
static char *opt_MT;
static char *opt_o;
static char *opt_ksas;
static int opt_stack = 4096;

static StringArray std_include_paths;

char *base_file;
static char *output_file;

static StringArray input_paths;
static StringArray tmpfiles;

static void usage(int status) {
  FILE *out = status ? stderr : stdout;
  fprintf(out,
    "uso: kscc [opcoes] arquivo...\n"
    "\n"
    "kscc: compilador C para a maquina virtual KSVM v2.\n"
    "Fork do chibicc (https://github.com/rui314/chibicc), (c) Rui Ueyama, licenca MIT.\n"
    "\n"
    "  -o <arq>           saida (padrao: a.ksx; com -c: .kso; com -S: .ksasm)\n"
    "  -S                 so gera o assembly .ksasm\n"
    "  -c                 compila e valida a unidade -> .kso (texto .ksasm)\n"
    "  -E                 so o preprocessador\n"
    "  -I <dir>           acrescenta diretorio de include\n"
    "  -D NOME[=val]      define macro\n"
    "  -U NOME            remove macro\n"
    "  -include <arq>     inclui <arq> antes do fonte\n"
    "  -x c|assembler|none  tipo dos arquivos seguintes\n"
    "  -nostdlib          nao acrescenta crt0 nem libks no link\n"
    "  -g                 passa -g ao ksas (tabela de simbolos completa)\n"
    "  -Wl,--stack=N      tamanho da pilha do programa (padrao 4096)\n"
    "  -Wl,--no-gc-sections  liga a libks inteira (padrao: so o que o\n"
    "                     programa usa)\n"
    "  --ksas <caminho>   ksas a usar (padrao: $KSAS, $KSVM2_HOME/build/ksas)\n"
    "  -###               mostra os comandos sem executa-los\n"
    "  -M -MD -MMD -MF -MP -MT -MQ  dependencias para o make\n"
    "  --help             esta ajuda\n");
  exit(status);
}

static bool take_arg(char *arg) {
  char *x[] = {
    "-o", "-I", "-idirafter", "-include", "-x", "-MF", "-MT", "-MQ",
    "-D", "-U", "--ksas",
  };

  for (int i = 0; i < (int)(sizeof(x) / sizeof(*x)); i++)
    if (!strcmp(arg, x[i]))
      return true;
  return false;
}

static bool endswith(char *p, char *q) {
  int len1 = strlen(p);
  int len2 = strlen(q);
  return (len1 >= len2) && !strcmp(p + len1 - len2, q);
}

// Returns true if a given file exists.
bool file_exists(char *path) {
  struct stat st;
  return !stat(path, &st);
}

// kscc: diretório do próprio executável.
static char *self_dir(char *argv0) {
  char *self = realpath("/proc/self/exe", NULL);
  if (!self)
    self = realpath(argv0, NULL);
  if (!self)
    self = strdup(argv0);
  return dirname(self);
}

// kscc: o "prefixo" é o diretório que contém include/ (headers do
// compilador) e libks/include/ (headers da libks). Procura subindo a
// partir do diretório do binário, o que cobre build/kscc e
// build/asan/kscc dentro do repositório. $KSCC_PREFIX tem precedência.
static char *prefix_dir;

static char *find_prefix(char *argv0) {
  if (prefix_dir)
    return prefix_dir;

  char *env = getenv("KSCC_PREFIX");
  if (env && *env)
    return prefix_dir = env;

  char *start = self_dir(argv0);
  char *dir = start;
  for (int i = 0; i < 4; i++) {
    dir = dirname(strdup(dir));
    if (file_exists(format("%s/include/stddef.h", dir)) &&
        file_exists(format("%s/libks/include/stdio.h", dir)))
      return prefix_dir = dir;
  }

  // Fallback: <dir do binário>/.. mesmo sem os arquivos, para que as
  // mensagens de erro mostrem onde eles eram esperados.
  return prefix_dir = dirname(strdup(start));
}

// kscc: diretório com crt0.ksasm e libks.kso, gerados por libks/Makefile
// ao lado do compilador que os compilou: build/kscc usa build/libks e
// build/asan/kscc usa build/asan/libks. $KSCC_LIBDIR tem precedência.
static char *find_libdir(char *argv0) {
  char *env = getenv("KSCC_LIBDIR");
  if (env && *env)
    return env;
  return format("%s/libks", self_dir(argv0));
}

static void add_default_include_paths(char *argv0) {
  // kscc: só os headers do próprio kscc, depois dos -I: primeiro os da
  // libks (stdio.h, ...), depois os do compilador (stddef.h, stdarg.h,
  // ...). Nunca /usr/include (LP64/glibc).
  strarray_push(&include_paths, format("%s/libks/include", find_prefix(argv0)));
  strarray_push(&include_paths, format("%s/include", find_prefix(argv0)));

  // Keep a copy of the standard include paths for -MMD option.
  for (int i = 0; i < include_paths.len; i++)
    strarray_push(&std_include_paths, include_paths.data[i]);
}

static void define(char *str) {
  char *eq = strchr(str, '=');
  if (eq)
    define_macro(strndup(str, eq - str), eq + 1);
  else
    define_macro(str, "1");
}

static FileType parse_opt_x(char *s) {
  if (!strcmp(s, "c"))
    return FILE_C;
  if (!strcmp(s, "assembler") || !strcmp(s, "ksasm"))
    return FILE_ASM;
  if (!strcmp(s, "none"))
    return FILE_NONE;
  error("<linha de comando>: argumento desconhecido para -x: %s", s);
}

static char *quote_makefile(char *s) {
  char *buf = calloc(1, strlen(s) * 2 + 1);

  for (int i = 0, j = 0; s[i]; i++) {
    switch (s[i]) {
    case '$':
      buf[j++] = '$';
      buf[j++] = '$';
      break;
    case '#':
      buf[j++] = '\\';
      buf[j++] = '#';
      break;
    case ' ':
    case '\t':
      for (int k = i - 1; k >= 0 && s[k] == '\\'; k--)
        buf[j++] = '\\';
      buf[j++] = '\\';
      buf[j++] = s[i];
      break;
    default:
      buf[j++] = s[i];
      break;
    }
  }
  return buf;
}

// kscc: -Wl,opção[,opção...]. Existem --stack=N (rodapé .stack do link)
// e --gc-sections/--no-gc-sections (seleção por uso da libks, src/link.c).
static void parse_wl(char *arg) {
  char *s = strdup(arg + 4);
  for (char *opt = strtok(s, ","); opt; opt = strtok(NULL, ",")) {
    if (!strcmp(opt, "--gc-sections") || !strcmp(opt, "--no-gc-sections")) {
      opt_no_gc = !strcmp(opt, "--no-gc-sections");
      continue;
    }

    char *val = NULL;
    if (!strncmp(opt, "--stack=", 8))
      val = opt + 8;
    else if (!strcmp(opt, "--stack"))
      val = strtok(NULL, ",");

    if (!val)
      error("<linha de comando>: opcao de link desconhecida: %s", opt);

    char *end;
    long n = strtol(val, &end, 0);
    if (*end || n <= 0 || n > 0x10000000 || n % 4)
      error("<linha de comando>: --stack espera um numero positivo multiplo de 4: %s", val);
    opt_stack = n;
  }
}

static void parse_args(int argc, char **argv) {
  // Make sure that all command line options that take an argument
  // have an argument.
  for (int i = 1; i < argc; i++)
    if (take_arg(argv[i]))
      if (!argv[++i])
        usage(1);

  StringArray idirafter = {0};

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-###")) {
      opt_hash_hash_hash = true;
      continue;
    }

    if (!strcmp(argv[i], "-cc1")) {
      opt_cc1 = true;
      continue;
    }

    if (!strcmp(argv[i], "--help"))
      usage(0);

    if (!strcmp(argv[i], "-o")) {
      opt_o = argv[++i];
      continue;
    }

    if (!strncmp(argv[i], "-o", 2)) {
      opt_o = argv[i] + 2;
      continue;
    }

    if (!strcmp(argv[i], "-S")) {
      opt_S = true;
      continue;
    }

    if (!strcmp(argv[i], "-fcommon")) {
      opt_fcommon = true;
      continue;
    }

    if (!strcmp(argv[i], "-fno-common")) {
      opt_fcommon = false;
      continue;
    }

    if (!strcmp(argv[i], "-c")) {
      opt_c = true;
      continue;
    }

    if (!strcmp(argv[i], "-E")) {
      opt_E = true;
      continue;
    }

    if (!strcmp(argv[i], "-I")) {
      strarray_push(&include_paths, argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-I", 2)) {
      strarray_push(&include_paths, argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-D")) {
      define(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-D", 2)) {
      define(argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-U")) {
      undef_macro(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-U", 2)) {
      undef_macro(argv[i] + 2);
      continue;
    }

    if (!strcmp(argv[i], "-include")) {
      strarray_push(&opt_include, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-x")) {
      opt_x = parse_opt_x(argv[++i]);
      continue;
    }

    if (!strncmp(argv[i], "-x", 2)) {
      opt_x = parse_opt_x(argv[i] + 2);
      continue;
    }

    if (!strncmp(argv[i], "-Wl,", 4)) {
      parse_wl(argv[i]);
      continue;
    }

    if (!strcmp(argv[i], "-nostdlib")) {
      opt_nostdlib = true;
      continue;
    }

    if (!strcmp(argv[i], "--ksas")) {
      opt_ksas = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-M")) {
      opt_M = true;
      continue;
    }

    if (!strcmp(argv[i], "-MF")) {
      opt_MF = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-MP")) {
      opt_MP = true;
      continue;
    }

    if (!strcmp(argv[i], "-MT")) {
      if (opt_MT == NULL)
        opt_MT = argv[++i];
      else
        opt_MT = format("%s %s", opt_MT, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-MD")) {
      opt_MD = true;
      continue;
    }

    if (!strcmp(argv[i], "-MQ")) {
      if (opt_MT == NULL)
        opt_MT = quote_makefile(argv[++i]);
      else
        opt_MT = format("%s %s", opt_MT, quote_makefile(argv[++i]));
      continue;
    }

    if (!strcmp(argv[i], "-MMD")) {
      opt_MD = opt_MMD = true;
      continue;
    }

    if (!strcmp(argv[i], "-fpic") || !strcmp(argv[i], "-fPIC")) {
      // kscc: aceito e ignorado; o ksas tem .pic, mas o link por
      // concatenação ainda não precisa dele.
      opt_fpic = true;
      continue;
    }

    if (!strcmp(argv[i], "-cc1-input")) {
      base_file = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-cc1-output")) {
      output_file = argv[++i];
      continue;
    }

    if (!strcmp(argv[i], "-idirafter")) {
      strarray_push(&idirafter, argv[++i]);
      continue;
    }

    if (!strcmp(argv[i], "-hashmap-test")) {
      hashmap_test();
      exit(0);
    }

    if (!strcmp(argv[i], "-g")) {
      opt_g = true;
      continue;
    }

    if (!strcmp(argv[i], "-ffreestanding")) {
      undef_macro("__STDC_HOSTED__");
      define_macro("__STDC_HOSTED__", "0");
      continue;
    }

    // kscc: opções de link do host não têm equivalente na KSVM.
    if (!strncmp(argv[i], "-l", 2) || !strncmp(argv[i], "-L", 2) ||
        !strcmp(argv[i], "-static") || !strcmp(argv[i], "-shared") ||
        !strcmp(argv[i], "-Xlinker"))
      error("opcao nao suportada nesta versao do kscc: %s", argv[i]);

    // These options are ignored for now.
    if (!strncmp(argv[i], "-O", 2) ||
        !strncmp(argv[i], "-W", 2) ||
        !strncmp(argv[i], "-g", 2) ||
        !strncmp(argv[i], "-std=", 5) ||
        !strcmp(argv[i], "-fno-builtin") ||
        !strcmp(argv[i], "-fno-omit-frame-pointer") ||
        !strcmp(argv[i], "-fno-stack-protector") ||
        !strcmp(argv[i], "-fno-strict-aliasing") ||
        !strcmp(argv[i], "-w"))
      continue;

    if (argv[i][0] == '-' && argv[i][1] != '\0')
      error("argumento desconhecido: %s", argv[i]);

    strarray_push(&input_paths, argv[i]);
  }

  for (int i = 0; i < idirafter.len; i++)
    strarray_push(&include_paths, idirafter.data[i]);

  if (input_paths.len == 0)
    error("nenhum arquivo de entrada");

  // -E implies that the input is the C macro language.
  if (opt_E)
    opt_x = FILE_C;
}

static FILE *open_file(char *path) {
  if (!path || strcmp(path, "-") == 0)
    return stdout;

  FILE *out = fopen(path, "w");
  if (!out)
    error("nao foi possivel abrir o arquivo de saida: %s: %s", path, strerror(errno));
  return out;
}

// Replace file extension
static char *replace_extn(char *tmpl, char *extn) {
  char *filename = basename(strdup(tmpl));
  char *dot = strrchr(filename, '.');
  if (dot)
    *dot = '\0';
  return format("%s%s", filename, extn);
}

static void cleanup(void) {
  for (int i = 0; i < tmpfiles.len; i++)
    unlink(tmpfiles.data[i]);
}

char *create_tmpfile(void) {
  char *path = strdup("/tmp/kscc-XXXXXX");
  int fd = mkstemp(path);
  if (fd == -1)
    error("mkstemp falhou: %s", strerror(errno));
  close(fd);

  strarray_push(&tmpfiles, path);
  return path;
}

bool driver_dry_run(void) {
  return opt_hash_hash_hash;
}

static void print_command(char **argv) {
  fprintf(stderr, "%s", argv[0]);
  for (int i = 1; argv[i]; i++)
    fprintf(stderr, " %s", argv[i]);
  fprintf(stderr, "\n");
}

// kscc: com -### os comandos são mostrados e não executados (como o gcc;
// o chibicc mostrava e executava).
int run_subprocess_capture(char **argv, char *errfile) {
  if (opt_hash_hash_hash) {
    print_command(argv);
    return 0;
  }

  fflush(stdout);
  fflush(stderr);

  pid_t pid = fork();
  if (pid == -1)
    error("fork falhou: %s", strerror(errno));

  if (pid == 0) {
    // Child process. Run a new command.
    if (errfile) {
      int fd = open(errfile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (fd != -1) {
        dup2(fd, 2);
        close(fd);
      }
    }
    execvp(argv[0], argv);
    fprintf(stderr, "kscc: falha ao executar %s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }

  // Wait for the child process to finish.
  int status;
  while (waitpid(pid, &status, 0) == -1)
    if (errno != EINTR)
      error("waitpid falhou: %s", strerror(errno));

  if (WIFEXITED(status))
    return WEXITSTATUS(status);

  // kscc: um subprocesso morto por sinal (cc1 com SIGSEGV, ksas abortado)
  // fazia o driver sair com 1 sem mensagem nenhuma.
  if (WIFSIGNALED(status)) {
    char *what = argv[0];
    for (int i = 1; argv[i]; i++)
      if (!strcmp(argv[i], "-cc1-input") && argv[i + 1])
        what = format("cc1 (%s)", argv[i + 1]);
    fprintf(stderr, "kscc: %s terminou pelo sinal %d (%s)\n", what,
            WTERMSIG(status), strsignal(WTERMSIG(status)));
    return 128 + WTERMSIG(status);
  }
  return 128;
}

static void run_subprocess(char **argv) {
  if (run_subprocess_capture(argv, NULL) != 0)
    exit(1);
}

static void run_cc1(int argc, char **argv, char *input, char *output) {
  char **args = calloc(argc + 10, sizeof(char *));
  memcpy(args, argv, argc * sizeof(char *));
  args[argc++] = "-cc1";

  if (input) {
    args[argc++] = "-cc1-input";
    args[argc++] = input;
  }

  if (output) {
    args[argc++] = "-cc1-output";
    args[argc++] = output;
  }

  run_subprocess(args);
}

// Print tokens to stdout. Used for -E.
static void print_tokens(Token *tok) {
  FILE *out = open_file(opt_o ? opt_o : "-");

  int line = 1;
  for (; tok->kind != TK_EOF; tok = tok->next) {
    if (line > 1 && tok->at_bol)
      fprintf(out, "\n");
    if (tok->has_space && !tok->at_bol)
      fprintf(out, " ");
    fprintf(out, "%.*s", tok->len, tok->loc);
    line++;
  }
  fprintf(out, "\n");
  if (out != stdout)
    fclose(out);
}

static bool in_std_include_path(char *path) {
  for (int i = 0; i < std_include_paths.len; i++) {
    char *dir = std_include_paths.data[i];
    int len = strlen(dir);
    if (strncmp(dir, path, len) == 0 && path[len] == '/')
      return true;
  }
  return false;
}

// If -M options is given, the compiler write a list of input files to
// stdout in a format that "make" command can read. This feature is
// used to automate file dependency management.
static void print_dependencies(void) {
  char *path;
  if (opt_MF)
    path = opt_MF;
  else if (opt_MD)
    path = replace_extn(opt_o ? opt_o : base_file, ".d");
  else if (opt_o)
    path = opt_o;
  else
    path = "-";

  FILE *out = open_file(path);
  if (opt_MT)
    fprintf(out, "%s:", opt_MT);
  else
    fprintf(out, "%s:", quote_makefile(replace_extn(base_file, ".kso")));

  File **files = get_input_files();

  for (int i = 0; files[i]; i++) {
    if (opt_MMD && in_std_include_path(files[i]->name))
      continue;
    fprintf(out, " \\\n  %s", files[i]->name);
  }

  fprintf(out, "\n\n");

  if (opt_MP) {
    for (int i = 1; files[i]; i++) {
      if (opt_MMD && in_std_include_path(files[i]->name))
        continue;
      fprintf(out, "%s:\n\n", quote_makefile(files[i]->name));
    }
  }
  if (out != stdout)
    fclose(out);
}

static Token *must_tokenize_file(char *path) {
  Token *tok = tokenize_file(path);
  if (!tok)
    error("%s: %s", path, strerror(errno));
  return tok;
}

static Token *append_tokens(Token *tok1, Token *tok2) {
  if (!tok1 || tok1->kind == TK_EOF)
    return tok2;

  Token *t = tok1;
  while (t->next->kind != TK_EOF)
    t = t->next;
  t->next = tok2;
  return tok1;
}

// kscc: identificador da unidade (doc 02 §5): FNV-1a de 32 bits do caminho
// absoluto do fonte. Entra nos nomes de símbolos static (_S<id>_nome) e de
// todos os labels gerados (L<id>.else.3), que precisam ser únicos depois da
// concatenação de várias unidades no link.
static char *make_unit_id(char *path) {
  char *abs = strcmp(path, "-") ? realpath(path, NULL) : NULL;
  char *s = abs ? abs : path;

  uint32_t h = 2166136261u;
  for (unsigned char *p = (unsigned char *)s; *p; p++) {
    h ^= *p;
    h *= 16777619u;
  }
  free(abs);
  return format("%08x", h);
}

static void cc1(void) {
  unit_id = make_unit_id(base_file);

  Token *tok = NULL;

  // Process -include option
  for (int i = 0; i < opt_include.len; i++) {
    char *incl = opt_include.data[i];

    char *path;
    if (file_exists(incl)) {
      path = incl;
    } else {
      path = search_include_paths(incl);
      if (!path)
        error("-include: %s: %s", incl, strerror(errno));
    }

    Token *tok2 = must_tokenize_file(path);
    tok = append_tokens(tok, tok2);
  }

  // Tokenize and parse.
  Token *tok2 = must_tokenize_file(base_file);
  tok = append_tokens(tok, tok2);
  tok = preprocess(tok);

  // If -M or -MD are given, print file dependencies.
  if (opt_M || opt_MD) {
    print_dependencies();
    if (opt_M)
      return;
  }

  // If -E is given, print out preprocessed C code as a result.
  if (opt_E) {
    print_tokens(tok);
    return;
  }

  Obj *prog = parse(tok);

  // Open a temporary output buffer.
  char *buf;
  size_t buflen;
  FILE *output_buf = open_memstream(&buf, &buflen);

  // Traverse the AST to emit assembly.
  codegen(prog, output_buf);
  fclose(output_buf);

  // Write the asembly text to a file.
  FILE *out = open_file(output_file);
  fwrite(buf, buflen, 1, out);
  if (out != stdout)
    fclose(out);
}

// kscc: ksas a usar. Ordem: --ksas, $KSAS, $KSVM2_HOME/build/ksas,
// caminho do Makefile embutido no binário, "ksas" no PATH.
static char *ksas_path(void) {
  if (opt_ksas)
    return opt_ksas;

  char *env = getenv("KSAS");
  if (env && *env)
    return env;

  env = getenv("KSVM2_HOME");
  if (env && *env)
    return format("%s/build/ksas", env);

#ifdef KSCC_DEFAULT_KSAS
  if (file_exists(KSCC_DEFAULT_KSAS))
    return KSCC_DEFAULT_KSAS;
#endif
  return "ksas";
}

static LinkOptions link_options(char *argv0) {
  LinkOptions o = {0};
  o.ksas = ksas_path();
  o.libdir = find_libdir(argv0);
  o.nostdlib = opt_nostdlib;
  o.no_gc = opt_no_gc;
  o.debug = opt_g;
  o.stack_size = opt_stack;
  return o;
}

// kscc: "montar" uma unidade = validar com o ksas e copiar o texto para o
// .kso (doc 04 §3).
static void assemble(char *argv0, char *input, char *display, char *output) {
  LinkOptions o = link_options(argv0);
  ks_check_unit(&o, input, display);
  ks_copy_file(input, output);
}

static void run_linker(char *argv0, StringArray *inputs, StringArray *names, char *output) {
  LinkOptions o = link_options(argv0);
  ks_link(&o, inputs, names, output);
}

static FileType get_file_type(char *filename) {
  if (opt_x != FILE_NONE)
    return opt_x;

  if (endswith(filename, ".kso"))
    return FILE_OBJ;
  if (endswith(filename, ".c"))
    return FILE_C;
  if (endswith(filename, ".ksasm") || endswith(filename, ".s"))
    return FILE_ASM;

  error("<linha de comando>: extensao de arquivo desconhecida: %s", filename);
}

int main(int argc, char **argv) {
  atexit(cleanup);
  init_macros();
  parse_args(argc, argv);

  if (opt_cc1) {
    add_default_include_paths(argv[0]);
    cc1();
    return 0;
  }

  if (input_paths.len > 1 && opt_o && (opt_c || opt_S || opt_E))
    error("nao e possivel usar '-o' com '-c', '-S' ou '-E' e varios arquivos");

  // kscc: como o gcc, recusa uma saída que é um dos arquivos de entrada
  // (`kscc -S -o x.c x.c` sobrescrevia o fonte com o assembly).
  struct stat so, si;
  if (opt_o && strcmp(opt_o, "-") && !stat(opt_o, &so))
    for (int i = 0; i < input_paths.len; i++)
      if (!stat(input_paths.data[i], &si) && si.st_dev == so.st_dev &&
          si.st_ino == so.st_ino)
        error("o arquivo de entrada '%s' e o mesmo que o de saida", input_paths.data[i]);

  // kscc: objetos (.kso) a concatenar no link, com o nome de exibição de
  // cada um para as mensagens de erro do ksas.
  StringArray ld_args = {0};
  StringArray ld_names = {0};

  for (int i = 0; i < input_paths.len; i++) {
    char *input = input_paths.data[i];

    if (strcmp(input, "-") && !opt_hash_hash_hash && !file_exists(input))
      error("%s: arquivo nao encontrado", input);

    char *output;
    if (opt_o)
      output = opt_o;
    else if (opt_S)
      output = replace_extn(input, ".ksasm");
    else
      output = replace_extn(input, ".kso");

    FileType type = get_file_type(input);

    // kscc: assembly ou .kso vindo de stdin ("-") vai para um temporário,
    // já que o ksas e o link leem arquivos.
    if (!strcmp(input, "-") && (type == FILE_ASM || type == FILE_OBJ) &&
        !opt_hash_hash_hash) {
      char *tmp = create_tmpfile();
      ks_copy_file("/dev/stdin", tmp);
      input = tmp;
    }

    // Handle .kso
    if (type == FILE_OBJ) {
      if (opt_c || opt_S || opt_E)
        continue;
      strarray_push(&ld_args, input);
      strarray_push(&ld_names, input);
      continue;
    }

    // Handle .ksasm
    if (type == FILE_ASM) {
      if (opt_S || opt_E)
        continue;
      if (opt_c) {
        assemble(argv[0], input, input, output);
        continue;
      }
      strarray_push(&ld_args, input);
      strarray_push(&ld_names, input);
      continue;
    }

    assert(type == FILE_C);

    // Just preprocess
    if (opt_E || opt_M) {
      run_cc1(argc, argv, input, NULL);
      continue;
    }

    // Compile
    if (opt_S) {
      run_cc1(argc, argv, input, output);
      continue;
    }

    // Compile and assemble
    if (opt_c) {
      char *tmp = create_tmpfile();
      run_cc1(argc, argv, input, tmp);
      assemble(argv[0], tmp, format("%s(.ksasm)", input), output);
      continue;
    }

    // Compile, assemble and link
    // kscc: sem validação por unidade aqui: os erros do ksas no link já
    // são atribuídos ao arquivo de origem (ver link.c).
    char *tmp = create_tmpfile();
    run_cc1(argc, argv, input, tmp);
    strarray_push(&ld_args, tmp);
    // Linhas citadas pelo ksas são do assembly gerado (que tem comentários
    // "; arquivo:linha" apontando para o C).
    strarray_push(&ld_names, format("%s(.ksasm)", input));
  }

  if (ld_args.len > 0)
    run_linker(argv[0], &ld_args, &ld_names, opt_o ? opt_o : "a.ksx");
  return 0;
}
