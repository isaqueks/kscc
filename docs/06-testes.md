# 06 — Testes

## 1. Camadas

| Camada | O que é | Oráculo |
|--------|---------|---------|
| **Unitários do compilador** (`test/unit/`) | Programas C de 5 a 50 linhas, um por construção do codegen (doc 03 §3). Compilados → montados → executados no `ksvm` | stdout e código de saída esperados, escritos à mão no cabeçalho do `.c` (`// expect: 42`) |
| **Suite do chibicc** (`test/chibicc/`) | Os 49 arquivos originais adaptados (§2) | Cada arquivo usa `ASSERT(esperado, expr)`; passa se termina com exit 0 e imprime `OK` |
| **Diferencial** (`test/diff/`) | Programas C compilados com `kscc` e rodados no `ksvm`, e compilados com `gcc -m32` e rodados nativamente; saídas comparadas | o gcc. Pega bugs de semântica (promoções, sinal, overflow definido para unsigned, ordem de avaliação não importa porque os programas evitam UB) |
| **Fuzz diferencial** (`test/fuzz/`) | Gerador de programas C aleatórios sem UB (Csmith com `--no-float --no-longlong --no-packed-struct`, se disponível no WSL; senão um gerador próprio de expressões inteiras) | gcc `-m32` |
| **Bootstrap parcial** | A `libks` compilada pelo `kscc` e exercitada pela suite | a própria suite |
| **Embarcado** | 3 programas C (`hello`, `blink`, `selftest` com 50 asserts) compilados pelo `kscc` e rodados em simavr e QEMU via os alvos `avr-custom`/`arm-custom` da ksvm2 | UART esperada |

Tudo acionado por `make test`; `make test-all` inclui fuzz e embarcado.

## 2. Adaptação da suite do chibicc

Os testes originais assumem x86-64 LP64 e glibc. Mudanças por categoria:

| Arquivo(s) | Mudança |
|------------|---------|
| `sizeof.c`, `alignof.c`, `usualconv.c`, `cast.c`, `arith.c`, `pointer.c`, `literal.c`, `constexpr.c` | Valores esperados para `long` = 4, ponteiro = 4; trechos com `long long` / float ficam atrás de `#ifdef KSCC_HAS_LL` / `KSCC_HAS_FLOAT` (definidos pelo driver quando as fases 5/6 existirem). Os literais `1L << 40` etc. são removidos ou movidos para `KSCC_HAS_LL` |
| `float.c` | inteiro atrás de `KSCC_HAS_FLOAT` (fase 6) |
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
