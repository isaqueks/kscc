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
| `stdlib.h` | `abs labs atoi atol strtol strtoul strtof strtod strtold atof rand srand exit abort qsort bsearch malloc calloc realloc free` | `exit` chama `atexit` (até 8 handlers) e `ks_sys_exit`. `strtof` e família: §2.1 |
| `stdio.h` | `putchar getchar puts gets_s fputs printf sprintf snprintf vprintf vsprintf vsnprintf fprintf(stdout/stderr) fflush` | `printf` suporta `%d %i %u %x %X %o %c %s %p %%` e `%f %F %e %E %g %G %a %A` (§2.1), flags `- 0 + espaço #`, largura, precisão, `l` (igual a `int`), `h`, `hh`, `z`, `L` (só nas de ponto flutuante); **sem** `%lld` até a fase 5. Saída bufferizada em 128 bytes, descarregada em `\n`, `fflush` e `exit` |
| `math.h` | `sqrt fabs floor ceil trunc rint nearbyint fmin fmax round fmod copysign` (e as versões `f` e `l`, iguais), `isnan isinf isfinite signbit fpclassify isnormal`, `isgreater isgreaterequal isless islessequal islessgreater isunordered`, `HUGE_VAL HUGE_VALF HUGE_VALL INFINITY NAN`, `FP_*`, `M_PI` e as demais `M_*` | As que são uma instrução F (`sqrt`...`fmax`) e as classificações são macros para os builtins do kscc (doc 03 §1, ponto flutuante), sem chamada; as funções existem também (`(sqrtf)(x)`, `&sqrtf`). `round` (empate para longe de zero, sem o erro do `floor(x + 0.5)`) e `fmod` (exato, aritmética inteira sobre as mantissas) são C. `math_errhandling` 0: não há errno nem exceções. **Sem** `sin cos tan exp log pow` e afins nesta fase |
| `assert.h` | `assert(x)` → mensagem `arquivo:linha: assert(x) falhou` + saída com código 1 (sem `atexit`). `abort()` continua sendo `trap 1` (fault TRAP, código 78 no runner) | |
| `errno.h` | `errno` (= `ks_errno`), `ENOSYS EINVAL EIO EAGAIN ENOMEM` mapeados dos `KSVM_ERR_*` | |
| `stdarg.h` | `va_list va_start va_arg va_end va_copy` | `va_list` = `char*`; `va_start` é builtin do compilador (doc 03) |
| `stddef.h` `stdint.h` `stdbool.h` `limits.h` `stdalign.h` `stdnoreturn.h` `iso646.h` | tipos e constantes ILP32 | `int64_t` só na fase 5 |
| `float.h` (do compilador) | `FLT_*`, `DBL_*`, `LDBL_*` | todos de binary32 (doc 02 §1.1): `DBL_EPSILON` = `FLT_EPSILON` = 2⁻²³, `DBL_DIG` 6, `DECIMAL_DIG` 9 |
| `ksvm/syscall.h` | protótipos das `ks_sys_*` + `struct ks_mem_info` | específico da KSVM |
| `ksvm/time.h` | `ks_millis`, `ks_sleep_ms` | |
| `ksvm/gpio.h` | `ks_pin_mode`, `ks_pin_write`, `ks_pin_read`, `ks_analog_read`, `ks_pwm_write` | `static inline`; retornam −1 com `errno = ENOSYS` se a HAL não implementa a syscall (`CAPS` da ksvm2 não enxerga o grupo 4, que é repassado à HAL, então não é consultado) |

Fora: `FILE*` genérico, `fopen` (grupo 3 pode ganhar um `ks_fopen` só no desktop), `time.h`
de calendário, locale, `setjmp` (possível em assembly: salvar `fp`, `sp`, endereço de retorno;
fica como extensão), `signal`, funções transcendentais de `math.h`.

### 2.1 Ponto flutuante na `libks`: `printf` e `strtof`

Ambos usam **só aritmética inteira de 32 bits** (o argumento de `%f` é lido como os bits do
binary32, `va_arg(ap, unsigned)`), então funcionam também numa VM sem a extensão F (porta AVR).

**`printf %f %e %g %a` — conversão exata.** Um binary32 finito é m·2ᵉ com m < 2²⁴; a sua
expansão decimal é finita (até 39 dígitos inteiros, 149 de fração, no máximo 112 significativos).
`float_digits` (`printf.c`) gera os dígitos exatos com um inteiro grande de limbs de 16 bits,
quatro por passada, e para no último que o arredondamento precisa (mais um dígito de "sticky");
o arredondamento para a precisão pedida é para o mais próximo com **empate para o par sobre o
valor exato**, como o glibc. Resultado: para qualquer float e qualquer precisão, os caracteres são
os mesmos que o gcc/glibc imprimem para esse float (promovido a double, que o representa
exatamente) — conferido em `test/diff/libks_float_printf.c` com 435 valores e 22 formatos cada.
Única diferença conhecida: o glibc 2.39 imprime `%#g` de 999999.5 como `1.e+06`, e a libks dá o
`1.00000e+06` que o C11 manda (o `#` mantém os zeros finais). Detalhes: precisão padrão 6; `%g`
segue C11 7.21.6.1p8 (estilo `e` se X < −4 ou X ≥ P); `%a` imprime a mantissa normalizada
(`0x1.99999ap-4`, também para subnormais, que são normais em double), sem zeros finais sem
precisão e arredondando (empate para o par, contando o dígito antes do ponto) com precisão;
`inf`/`nan` (`INF`/`NAN` em `F E G A`) levam o sinal do bit 31, inclusive `-nan`, e o flag `0` não
se aplica a eles. Custo: ~500 bytes de pilha e ~5–30 mil instruções por conversão (mais para
valores muito grandes ou muito pequenos).

Tamanho: a conversão tem ~20 KB de código (`printf("%.3f", x)` sozinho dá um `.ksx` de 35 KB, contra 15 KB do `hello`). Ela é uma **referência fraca** (doc 04 §3): só entra
no link quando o programa usa `printf` e alguma unidade passa `float`/`double` a uma função
variádica; senão `%f` consome os 4 bytes do argumento e imprime `<?>`, e o `hello` continua com
15 KB.

**`strtof`, `strtod`, `strtold`, `atof` — corretamente arredondadas** (as quatro são a mesma
conversão). Aceitam espaços iniciais, sinal, decimal com `.` e expoente `e`, hexadecimal `0x`
com `.` e expoente `p`, `inf`/`infinity` e `nan`/`nan(...)`, sem distinção de maiúsculas; sem
conversão devolvem 0 com `*end = s`. O valor decimal vira uma fração exata num/den de inteiros
grandes (até 640 bits), dividida até 26 bits de quociente mais o resto: o resultado é o binary32
mais próximo, empate para o par, inclusive subnormais e o limiar de overflow
(3.4028235677973366e38 → `FLT_MAX`, ...67e38 → inf). Os primeiros 120 dígitos significativos são
guardados e os seguintes só contam como "há algo não nulo", o que não muda o arredondamento (o
ponto médio entre dois binary32 tem no máximo 113 dígitos significativos); o expoente é limitado
a ±100.000. `errno = ERANGE` em overflow (±inf a partir de um número finito) e em underflow para
zero a partir de um valor não nulo; um resultado subnormal **não** liga `ERANGE` (o glibc liga
quando é inexato). O payload de `nan(...)` é ignorado: o resultado é sempre o NaN quieto
`0x7FC00000` (com o sinal pedido). Conferido contra o glibc em `test/diff/libks_strtof.c`
(literais de borda e ida e volta de 900 strings) e `test/unit/libks_strtof.c`.

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

## 4. Camada de teste (`test/chibicc/common.c`)

`assert_eq(int expected, int actual, const char *code)` usado pela suite do chibicc
(`test/common`), imprimindo `code => actual expected expected` e saindo com 1 na falha.

## 5. Build

`libks/Makefile`: compila cada `.c` com `kscc -c` → `.kso`, concatena com os `.ksasm` em
`build/libks/libks.kso` (ao lado do `kscc` que a compilou; `build/asan/libks/` no ASan). Os
headers ficam em `libks/include/`, procurados pelo driver após `-I`. Depende do `kscc` já construído; o `make` do
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
