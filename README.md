# kscc — compilador C para a KSVM v2

Fork do [chibicc](https://github.com/rui314/chibicc) (Rui Ueyama, MIT) com o gerador de
código retargetado para a máquina virtual [KSVM v2](https://github.com/isaqueks/ksvm2).

Estado: fases 0 a 4 (driver, link por concatenação, inteiros, controle de fluxo,
chamadas, ponteiros, structs/unions por valor, globais e `.rodata`, varargs,
`alloca`/VLA, símbolos comuns, biblioteca de runtime `libks` com `printf` e `malloc`) e
fase 6 (ponto flutuante). Ainda não: `long long` (fase 5). Plano completo em
`docs/README.md`.

## Ponto flutuante: `float`, `double` e `long double` são binary32

A KSVM só tem ponto flutuante de 32 bits (extensão F: IEEE 754 binary32 nos registradores
inteiros, arredondamento para o mais próximo, NaN canônico). Por isso **os três tipos têm 4
bytes e a mesma representação binary32** — divergência deliberada da norma, como o
`-fshort-double` do avr-gcc antigo (`docs/02-abi.md` §1.1):

- `sizeof(double) == 4`, `DBL_EPSILON == FLT_EPSILON`, `DBL_DIG == 6`; `0.1 + 0.2 == 0.3` é
  verdadeiro. Literais sem sufixo são binary32 (como `gcc -fsingle-precision-constant`).
- Os três continuam tipos distintos para `_Generic` e compatibilidade; `printf("%f", x)` e
  `%Lf` funcionam; `float` passa e volta como um `int` (r1–r4, pilha, retorno em r1).
- Tudo inline, sem rotinas de runtime: `fadd fsub fmul fdiv fneg fcmp itof utof ftoi ftou`
  e `fsqrt fabs ffloor fceil ftrunc frint fmin fmax` pelos builtins de `<math.h>`.
  Comparações corretas com NaN (só `!=` é verdadeiro); float → inteiro fora do alcance
  satura (é UB em C; o resultado da VM é definido e igual em todo host); as constantes
  dobradas pelo compilador dão os mesmos bits que a conta em tempo de execução.
- A porta **AVR não tem a extensão F**: lá uma operação de float dá `FAULT_BAD_OPCODE`
  (`ks_sys_caps(0x4000) & 4` diz se ela existe). `printf("%f")` e `strtof` usam só
  aritmética inteira e funcionam em qualquer porta.
- Sem `double` de 64 bits nem funções transcendentais (`sin`, `exp`, `log`, `pow`) por
  enquanto (`docs/07-roadmap.md`).

Depende da ksvm2 construída (`ksas`, `ksvm`) em `KSVM2_HOME` (padrão `../ksvm2`).

    make                # build/kscc e a libks (build/libks/)
    make test           # unitários (test/unit), examples/, suite do chibicc, driver.sh
    make test-asan      # idem com o compilador sob ASan/UBSan (e a libks compilada por ele)

## Compilando um programa

    build/kscc examples/hello.c -o hello.ksx
    ../ksvm2/build/tools/ksvm hello.ksx

    build/kscc -c a.c && build/kscc -c b.c      # unidades separadas (.kso)
    build/kscc a.kso b.kso -o prog.ksx

    echo '2 * (3 + 4)' | ../ksvm2/build/tools/ksvm calc.ksx   # stdin funciona

O driver liga sozinho o `crt0` e a `libks` (`-nostdlib` desliga os dois) e procura os
headers nesta ordem: `-I`, `libks/include/` (`stdio.h`, `stdlib.h`, ...), `include/`
(`stddef.h`, `stdarg.h`, `stdint.h`, ...). Nunca `/usr/include`. A `libks` compilada
fica ao lado do binário do compilador (`build/libks/libks.kso` e `crt0.ksasm`;
`$KSCC_LIBDIR` troca o lugar) e é recompilada pelo `make` sempre que o compilador
muda.

O link só inclui da `libks` o que o programa usa (cada função ou variável global é um
bloco; entram os blocos alcançáveis a partir das unidades do usuário). Uma função do
programa com o mesmo nome de uma da `libks` substitui a da biblioteca, como num `.a`
do Unix. `-Wl,--no-gc-sections` liga a `libks` inteira. Tamanhos do `.ksx`:

| Programa | Com seleção | `--no-gc-sections` |
|----------|------------:|-------------------:|
| `int main(void) { return 0; }` | 475 | — |
| `puts("oi")` | 2.524 | — |
| `examples/hello.c` (`printf`) | 15.315 | 86.878 |
| `printf("%.3f\n", 3.14159f)` | 35.022 | — |

A conversão de ponto flutuante do `printf` (~20 KB) só é ligada quando o programa passa
`float`/`double` a uma função variádica (referência fraca, `docs/04-driver-toolchain.md` §3).

Exemplos em `examples/` (`hello`, `fizzbuzz`, `linked_list`, `strings`, `calc`), com a
saída esperada no cabeçalho; `test/run.sh` executa todos.

## O que a `libks` cobre

C freestanding compilado pelo próprio kscc (`libks/src/`) mais wrappers de syscall em
assembly (`libks/asm/`). Especificação: `docs/05-libks.md`.

| Header | Conteúdo |
|--------|----------|
| `string.h` | `memcpy memmove memset memcmp memchr strlen strnlen strcpy strncpy strcat strncat strcmp strncmp strchr strrchr strstr strdup` |
| `ctype.h` | `isdigit isalpha isalnum isspace isupper islower isxdigit isprint toupper tolower` (só ASCII) |
| `stdlib.h` | `abs labs div ldiv atoi atol strtol strtoul strtof strtod strtold atof rand srand exit _Exit abort atexit` (8 handlers) `qsort bsearch malloc calloc realloc free`, e a extensão `ks_heap_check()` |
| `stdio.h` | `putchar getchar puts gets_s fgets fputs fputc putc fgetc getc fflush printf fprintf sprintf snprintf vprintf vfprintf vsprintf vsnprintf`; `stdin`/`stdout`/`stderr` são constantes |
| `math.h` | `sqrt fabs floor ceil trunc rint nearbyint fmin fmax round fmod copysign` (e `f`/`l`), `isnan isinf isfinite signbit fpclassify isnormal`, `isgreater`…`isunordered`, `HUGE_VAL INFINITY NAN`, `M_PI` etc. |
| `assert.h` | `assert` (falha: `arquivo:linha: assert(x) falhou` e saída com código 1) |
| `errno.h` | `errno`; `ENOSYS EFAULT EINVAL EIO EAGAIN` são os próprios `KSVM_ERR_*`, mais `EKSEOF ENOMEM ERANGE EDOM EILSEQ` |
| `ksvm/syscall.h` | `ks_sys_*` para todas as syscalls da KSVM (grupos 0 a 5) e `struct ks_mem_info` |
| `ksvm/time.h` | `ks_millis`, `ks_sleep_ms` |
| `ksvm/gpio.h` | `ks_pin_mode ks_pin_write ks_pin_read ks_analog_read ks_pwm_write` (−1 e `ENOSYS` sem GPIO) |

`include/` (do compilador) tem `stddef.h stdarg.h stdbool.h stdint.h limits.h stdalign.h
stdnoreturn.h iso646.h float.h`, todos ILP32 (`float.h` com `DBL_*` = `FLT_*`).

`printf`: `%d %i %u %x %X %o %c %s %p %n %%` e `%f %F %e %E %g %G %a %A`, flags
`- 0 + espaço #`, largura e precisão (também `*`), tamanhos `hh h l z t j` (todos de 32
bits) e `L`. A conversão de float é **exata** e arredonda como o glibc (empate para o par
sobre o valor exato): os caracteres saem iguais aos do gcc/glibc para qualquer precisão.
`strtof`/`strtod`/`atof` são **corretamente arredondadas** (decimal, hexadecimal, `inf`,
`nan`), com `ERANGE` em overflow e underflow para zero; o payload de `nan(...)` é
ignorado. `stdout` tem buffer
de 128 bytes, descarregado em `\n`, ao encher, em `fflush`, em `exit` (inclusive ao
voltar de `main`) e antes de ler `stdin`; `stderr` não tem buffer e sai no mesmo
console. `getchar` espera (cedendo 1 ms por vez) quando ainda não há entrada.

`malloc`: first-fit com lista de livres e coalescência dos vizinhos, cabeçalho de 8
bytes, alinhamento 4, heap entre o fim do `.bss` e `stack_limit - 64` (syscall
`MEM_INFO`). Com `make -C libks KS_MALLOC_DEBUG=1 KSCC=... OUT=...` os blocos liberados
são pintados com `0xDD` e os cabeçalhos conferidos (free duplo, `realloc` de bloco
livre, uso depois do free e cabeçalho corrompido chamam `abort`).

## O que a `libks` não cobre

- `%lld`/`long long` (fase 5): imprime `<?>` e não consome argumento (o compilador
  rejeita o tipo, então um argumento desses nunca chega ao `printf`).
- Funções transcendentais (`sin cos tan exp log pow`...), `double` de 64 bits,
  `fenv.h`, `complex.h`.
- `FILE*` genérico, `fopen` e arquivos (o grupo 3 de syscalls existe como `ks_sys_open`
  etc., só no runner de desktop), `scanf`, `ungetc`, `setvbuf`.
- `time.h` de calendário, locale, `setjmp`/`longjmp`, `signal`, `wchar.h`.
- `argc`/`argv`: a KSVM não tem syscall para eles; `main` recebe 0 e `NULL`.
