# 06 — Testes

## 1. Camadas

| Camada | O que é | Oráculo |
|--------|---------|---------|
| **Unitários do compilador** (`test/unit/`) | Programas C de 5 a 50 linhas, um por construção do codegen (doc 03 §3). Compilados → montados → executados no `ksvm` | stdout e código de saída esperados, escritos à mão no cabeçalho do `.c` (`// expect: 42`) |
| **Suite do chibicc** (`test/chibicc/`) | Os 49 arquivos originais adaptados (§2) | Cada arquivo usa `ASSERT(esperado, expr)`; passa se termina com exit 0 e imprime `OK` |
| **Diferencial** (`test/diff/`) | Programas C compilados com `kscc` e rodados no `ksvm`, e compilados com `gcc -m32` e rodados nativamente; saídas comparadas | o gcc. Pega bugs de semântica (promoções, sinal, overflow definido para unsigned, ordem de avaliação não importa porque os programas evitam UB) |
| **Fuzz diferencial** (`test/fuzz/`) | Gerador de programas C aleatórios sem UB (Csmith com `--no-float --no-longlong --no-packed-struct`, se disponível no WSL; e o gerador próprio `gen_expr.py`, em dois modos: expressões inteiras e, com `SEMENTE float`, programas só de `float`) | gcc `-m32` |
| **Bootstrap parcial** | A `libks` compilada pelo `kscc` e exercitada pela suite | a própria suite |
| **Embarcado** | 4 programas C (`hello`, `blink`, `selftest` com 50 asserts, `fselftest` com 32 de ponto flutuante) compilados pelo `kscc` e rodados em simavr e QEMU via os alvos `avr-custom`/`arm-custom` da ksvm2 | UART esperada |

Tudo acionado por `make test`; `make test-all` inclui fuzz e embarcado.

## 2. Adaptação da suite do chibicc

Os testes originais assumem x86-64 LP64 e glibc. Mudanças por categoria:

| Arquivo(s) | Mudança |
|------------|---------|
| `sizeof.c`, `alignof.c`, `usualconv.c`, `cast.c`, `arith.c`, `pointer.c`, `literal.c`, `constexpr.c` | Valores esperados para `long` = 4, ponteiro = 4, `double`/`long double` = 4; trechos com `long long` ficam atrás de `#ifdef KSCC_HAS_LL` (até a fase 5). Os literais `1L << 40` etc. são removidos ou movidos para `KSCC_HAS_LL` |
| `float.c` | ativo desde a fase 6: uma asserção muda (`(double)(unsigned long)-1` para `int` satura em `INT_MAX`, em vez do `INT_MIN` do x86-64) e cinco próprias do binary32 entram no fim. Os trechos que estavam atrás de `KSCC_HAS_FLOAT` nos demais arquivos (35 em 13 arquivos) voltaram, com `sizeof(double)`/`sizeof(long double)` = 4 e `(long)2e15` trocado por `(long)2e9` mais um caso de saturação |
| `tls.c`, `atomic.c`, `asm.c` | removidos da lista (não suportados; o compilador dá erro claro) |
| `alloca.c`, `vla.c` | mantidos; marcados "opcional" na fase 3 |
| `varargs.c` | reescrito para a `va_list` da KSVM; `sprintf`/`vsprintf` usam a `libks` |
| `stdhdr.c` | inclui só headers que a `libks` tem |
| `builtin.c`, `attribute.c` | subconjunto |
| `common` | `assert()` reimplementado sobre a `libks` (doc 05 §4); `printf` da `libks` |
| `driver.sh` | reescrito: testa `-S`, `-c`, `-o`, `-E`, `-I`, `-D`, `-include`, `-###`, erros de arquivo inexistente, com o `kscc` |
| `thirdparty/` | removido (compila git, sqlite etc.; fora do escopo) |

Regra de adaptação: cada mudança leva um comentário `// kscc: motivo`. Um script
`tools/diff_upstream.sh` mostra o diff contra o chibicc original para revisão.

## 3. Como um teste roda

```
kscc -Iinclude -Itest test/chibicc/arith.c test/chibicc/common.c -o build/arith.ksx
ksvm --max-insns 50000000 build/arith.ksx > build/arith.out ; echo $? > build/arith.rc
```
`--max-insns` evita que um laço infinito trave a suite. Faults aparecem como código 70+ e a
mensagem do `ksvm` é anexada ao relatório. Com `KSCC_TEST_TRACE=1` o harness reexecuta o teste
que falhou com `--trace` e guarda o trace.

## 3.1 Ponto flutuante (fase 6)

- **Unitários** (`test/unit/float_*.c`, `libks_printf_float.c`, `libks_strtof.c`, `err_float_*.c`):
  conferem os **bits** do binary32 (`float_test.h`, `EQB`), não comparações de float, para
  enxergar −0 e o padrão de NaN. Cobrem as operações (zeros com sinal, inf, NaN canônico,
  subnormais), as seis comparações com NaN em valor e em desvio, valor de verdade, casts com
  saturação, constantes dobradas x tempo de execução, `++/--`, ABI, builtins e `math.h`, tipos e
  `float.h`, `printf` e `strtof`, e os erros de compilação. As expectativas foram conferidas também
  no gcc `-m32 -msse -mfpmath=sse` (as diferenças são só as definidas pela VM).
- **Diferencial**: o lado do gcc usa `-msse -mfpmath=sse -fexcess-precision=standard
  -fsingle-precision-constant` (binary32 sem a precisão estendida do x87, literais sem sufixo em
  float) e `-lm`; o UBSan inclui `float-cast-overflow`, porque converter float fora do alcance é UB
  (o kscc satura, o x86 dá `INT_MIN`). Os programas `test/diff/float_*.c` só usam `float` (nunca
  `double`) e imprimem os bits com `test/fltout.h`, que escreve todo NaN como `nan`: o x86 produz
  `0xFFC00000` e a KSVM o canônico `0x7FC00000`. Também imprimem alguns `%f`/`%g`;
  `libks_float_printf.c` e `libks_strtof.c` comparam a libks com o glibc caractere a caractere.
- **Fuzz**: `gen_expr.py SEMENTE float` gera programas só com float (funções puras, ternário,
  comparações, builtins, `++/--`, compostas); float → inteiro só por `f2i/f2u`, que devolvem uma
  constante fora do alcance e para NaN; todo float impresso passa por `fb()` (NaN canônico); sem
  `fminf/fmaxf` (`fmax(-0, +0)` não é fixado pelo C). `run_fuzz.sh N` roda N inteiros e
  `FUZZ_FLOAT_N` (padrão N/2) float.
- **Embarcado**: `fselftest` roda no desktop e no Cortex-M (QEMU) e é comparado com o gcc; na
  porta AVR, sem a extensão F, ele detecta a falta por `CAPS(0x4000)` bit 2 e sai limpo, e os
  programas sem float continuam passando. Float em AVR não é suportado.

## 4. Testes do próprio compilador em C (host)

Reaproveitar o pouco que o chibicc tem (quase nada) e acrescentar testes de host para as partes
que não dependem da VM: tabela de casts (entrada → sequência de mnemônicos), escolha de offset
curto/longo em `gen_addr` (±8191), geração de nomes de símbolos (`_S<id>_`, `_L_`), e o
"linker" (concatenação, prefixos, detecção de duplicados). Harness: o mesmo `test.h` da ksvm2
copiado (sem dependência de build entre os projetos).

## 5. Sanitizers e flags

O kscc é código de host: compila com `-std=c11 -Wall -Wextra -Werror` (o chibicc usa
`-Wno-switch`; manter) e roda a suite sob `-fsanitize=address,undefined` no CI. Isso pega bugs
no compilador em si (o front-end do chibicc tem alguns `int` vs `long` que podem virar UB
quando `long` do alvo passa a ser 32 bits).

## 6. Métricas acompanhadas

- Contagem de arquivos da suite do chibicc passando (meta v1: todos os mantidos).
- Tamanho do `.ksx` da `libks` e de `hello.c` (regressão de tamanho de código).
- Instruções executadas por `test/bench/` (`fib(25)`, `sieve(10000)`, `qsort` de 2000 ints,
  `strlen` em laço) via `INSN_COUNT` — a métrica de desempenho que a fase 7 vai otimizar.
