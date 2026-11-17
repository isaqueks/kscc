# 05 — `libks`: biblioteca de runtime

Uma libc mínima, escrita em C e compilada pelo próprio `kscc` (o que também serve de teste de
bootstrap), mais alguns wrappers em `.ksasm`. Objetivo: o suficiente para a suite de testes do
chibicc e para programas úteis em microcontroladores. Não é POSIX e não finge ser.

## 1. Camada 0 — assembly (`libks/asm/`)

| Arquivo | Conteúdo |
|---------|----------|
| `crt0.ksasm` | `_start`: `clr r1` (argc=0), `clr r2` (argv=NULL), `call main`, `syscall EXIT` com `r1`. Se `main` retorna, o código de saída é o valor devolvido |
| `syscall.ksasm` | Uma função por syscall usada, respeitando a ABI do doc 02: `ks_sys_exit`, `ks_sys_putc`, `ks_sys_getc`, `ks_sys_puts(ptr,len)`, `ks_sys_gets(ptr,max)`, `ks_sys_millis`, `ks_sys_sleep_ms`, `ks_sys_mem_info(struct*)`, `ks_sys_random`, `ks_sys_caps(n)`, `ks_sys_pin_*`, `ks_sys_analog_read`, `ks_sys_open/close/read/write/seek` (grupo 3, só no desktop). Cada uma é `mov` dos argumentos + `syscall N` + `ret`; o erro em `r2` é copiado para a global `ks_errno` |
| `abort.ksasm` | `abort`: `trap 1`. `__builtin_trap` idem |

## 2. Camada 1 — C freestanding (`libks/src/`)

| Header | Funções | Observações |
|--------|---------|-------------|
| `string.h` | `memcpy memmove memset memcmp memchr strlen strnlen strcpy strncpy strcat strncat strcmp strncmp strchr strrchr strstr strdup` | `memcpy` word a word quando alinhado; é a função mais chamada pelo codegen de struct (não: o codegen copia inline; `memcpy` é só para o usuário) |
| `ctype.h` | `isdigit isalpha isalnum isspace isupper islower toupper tolower isxdigit isprint` | tabela de 128 bytes em `.rodata` |
| `stdlib.h` | `abs labs atoi atol strtol strtoul rand srand exit abort qsort bsearch malloc calloc realloc free` | `exit` chama `atexit` (até 8 handlers) e `ks_sys_exit` |
| `stdio.h` | `putchar getchar puts gets_s fputs printf sprintf snprintf vprintf vsprintf vsnprintf fprintf(stdout/stderr) fflush` | `printf` suporta `%d %i %u %x %X %o %c %s %p %%`, flags `- 0 +`, largura, precisão, `l` (igual a `int`), `h`, `hh`, `z`; **sem** `%f` até a fase 6, **sem** `%lld` até a fase 5. Saída bufferizada em 128 bytes, descarregada em `\n`, `fflush` e `exit` |
| `assert.h` | `assert(x)` → mensagem `arquivo:linha: assert(x) falhou` + `abort` | |
| `errno.h` | `errno` (= `ks_errno`), `ENOSYS EINVAL EIO EAGAIN ENOMEM` mapeados dos `KSVM_ERR_*` | |
| `stdarg.h` | `va_list va_start va_arg va_end va_copy` | `va_list` = `char*`; `va_start` é builtin do compilador (doc 03) |
| `stddef.h` `stdint.h` `stdbool.h` `limits.h` `stdalign.h` `stdnoreturn.h` `iso646.h` | tipos e constantes ILP32 | `int64_t` só na fase 5 |
| `ksvm/syscall.h` | protótipos das `ks_sys_*` + `struct ks_mem_info` | específico da KSVM |
| `ksvm/time.h` | `ks_millis`, `ks_sleep_ms` | |
| `ksvm/gpio.h` | `ks_pin_mode`, `ks_pin_write`, `ks_pin_read`, `ks_analog_read`, `ks_pwm_write` | retornam −1 se a HAL não suporta (`CAPS`) |

Fora: `FILE*` genérico, `fopen` (grupo 3 pode ganhar um `ks_fopen` só no desktop), `time.h`
de calendário, locale, `setjmp` (possível em assembly: salvar `fp`, `sp`, endereço de retorno;
fica como extensão), `signal`.

## 3. `malloc`

```
heap_start ──► [cabeçalho][bloco][cabeçalho][bloco] … ──► brk ──► … ◄── sp
                                                              stack_limit
```
- `ks_sys_mem_info` dá `heap_start` (fim do `.bss`) e `stack_limit`. O heap cresce de
  `heap_start` até `stack_limit - 64` (margem).
- Implementação: first-fit com lista encadeada de blocos livres e coalescência de vizinhos
  (~120 linhas). Cabeçalho de 8 bytes (`size | flag_livre`, `prev_size`). Alinhamento 4.
- Em AVR com 1 KB de RAM isso ainda funciona (é o mesmo algoritmo da avr-libc).
- `KS_MALLOC_DEBUG` (compile-time): preenche blocos liberados com `0xDD` e checa cabeçalhos.

## 4. Camada de teste (`libks/test/`)

`assert_eq(int expected, int actual, const char *code)` usado pela suite do chibicc
(`test/common`), imprimindo `code => actual expected expected` e saindo com 1 na falha.

## 5. Build

`libks/Makefile`: compila cada `.c` com `kscc -c` → `.kso`, concatena com os `.ksasm` em
`lib/libks.kso`, copia headers para `include/`. Depende do `kscc` já construído; o `make` do
projeto constrói o compilador primeiro. A `libks` é recompilada toda vez que o compilador muda,
o que a transforma em teste de regressão permanente de ~2.000 linhas de C real.

## 6. Tamanho esperado

| Componente | Linhas de C | Código KSVM estimado |
|------------|------------:|---------------------:|
| string + ctype | 400 | 3 KB |
| stdio (printf) | 500 | 5 KB |
| stdlib (malloc, qsort, strtol…) | 600 | 4 KB |
| asm (crt0, syscalls) | 150 | 1 KB |
| **total** | ~1.650 | ~13 KB |

Para o ATmega328P (32 KB de flash, 12 KB da VM) sobra ~20 KB para a imagem: cabe a `libks`
inteira mais um programa pequeno, mas o normal será compilar com `-nostdlib` e incluir só o que
se usa (ou a eliminação de código morto da fase 7).
