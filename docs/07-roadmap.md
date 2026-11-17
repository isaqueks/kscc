# 07 — Roadmap, pré-requisitos, estimativas e riscos

## 0. Pré-requisitos na ksvm2 (pequenos, fazer antes da fase 1)

| Item | Por quê | Esforço |
|------|---------|---------|
| Labels com `.` no `ksas` | Identificadores aceitam `.` no meio, mas um nome **começando** com `.` é lido como diretiva. O kscc gera `L<id>.else.N`, com `<id>` = hash FNV-1a do caminho da unidade (também evita colisão de contadores entre unidades no link) | feito (fase 1) |
| Limites do `ksas` | **Já suficientes**: `KSAS_MAX_SYMBOLS` 65.536, `KSAS_MAX_ITEMS` 1 M, pool de 16 MiB (`ir.h`), ajustáveis por `-D` na compilação do `ksas`. Só revisar se a suite inteira concatenada passar de 65 k símbolos | — |
| Validar `.kso` sem gerar saída | Não há `--syntax-only`; `ksas x.ksasm -o /dev/null` funciona no Linux. Opcional: adicionar `--syntax-only` para Windows | horas (opcional) |
| `.stack` e `.entry` podem vir do rodapé do link mesmo que o `.text` tenha vindo antes | Confirmar; o `ksas` já aceita diretivas em qualquer ordem | — |
| `ksvm --max-insns` e código de saída já existem | ✓ | — |
| Runner devolve `argc/argv`? | Não existe syscall para isso. `crt0` passa `argc=0`. Se quiser `argv`, acrescentar syscall `ARGS` no grupo 0 (fase 4, opcional) | dia |

## 1. Fases

### Fase 0 — Fork e esqueleto
- Clonar chibicc (commit `90d1f7f`, 2020-12-07), renomear para `kscc`, manter `LICENSE`.
- `Makefile` do projeto: constrói `kscc`, encontra `ksas`/`ksvm` (variável `KSVM2_HOME`,
  padrão `../ksvm2/build`), alvo `test`.
- `main.c`: `assemble()` chama `ksas`; `run_linker()` faz a concatenação; macros predefinidas.
- `codegen.c` esvaziado para um `emit_text` que gera só `main: halt` → `kscc -S` de um programa
  vazio produz `.ksasm` válido, `ksas` monta, `ksvm` executa com exit 0.

Critério: `int main(){ return 42; }` → `ksvm` sai com 42.

### Fase 1 — Inteiros, locais, controle de fluxo (núcleo do codegen)
- `type.c` ILP32. Erro explícito para `long long`, float, TLS, atomic, asm.
- `gen_expr`: `ND_NUM`, `ND_VAR` local, `ND_ASSIGN` escalar, binários inteiros, unários,
  comparações, `&&`/`||`/`!`, `ND_COND`, `ND_COMMA`, `ND_CAST` entre inteiros.
- `gen_stmt`: `if`, `for`/`while`/`do`, `return`, blocos, `goto`/labels, `switch`, `break`/
  `continue`.
- Prólogo/epílogo; `assign_lvar_offsets`.
- Chamadas de função com argumentos escalares (≤ 4 e > 4), recursão.

Critério: `test/unit` para cada item + `arith.c`, `control.c`, `function.c` (parte), `cast.c`,
`usualconv.c`, `variable.c` (parte) da suite passando.

### Fase 2 — Memória: ponteiros, globais, arrays, structs, strings, inicializadores
- `gen_addr` completo, `load`/`store` por tamanho, `ND_DEREF`/`ND_ADDR`/`ND_MEMBER`.
- `emit_data`: globais, `.rodata`, strings, `.word sym+off`, `.bss`, `.align`.
- Structs/unions: cópia, passagem por valor, retorno (≤ 4 em `r1`, maior via ponteiro oculto).
- `ND_MEMZERO`, inicializadores locais, literais compostos.
- Bitfields.
- Prefixo `_S<id>_` para `static`.

Critério: `pointer.c`, `struct.c`, `union.c`, `initializer.c`, `string.c`, `literal.c`,
`sizeof.c`, `alignof.c`, `offsetof.c`, `bitfield.c`, `complit.c`, `extern.c`, `commonsym.c`,
`variable.c` passando.

### Fase 3 — Varargs, alloca/VLA, builtins, resto da suite
- Convenção variádica (tudo na pilha), `va_start`/`va_arg` builtins, `varargs.c`.
- `alloca`/VLA (opcional, mas o chibicc os tem prontos; portar `builtin_alloca`).
- `_Generic`, `typeof`, `constexpr`, `enum`, `typedef`, `macro`, `include`, `line`,
  `pragma-once`, `unicode` (strings UTF-8/UTF-16/32: `.byte`/`.half`/`.word`), `decl`,
  `const`, `compat`, `generic`, `attribute` (subconjunto), `builtin` (subconjunto).
- `driver.sh` adaptado.

Critério: toda a suite mantida do chibicc passa (exceto float/ll atrás de `#ifdef`).

### Fase 4 — `libks` e programas reais
- `crt0`, wrappers de syscall, `string`, `ctype`, `stdlib` (com `malloc`), `stdio` (`printf`).
- `assert_eq` da suite passa a usar a `libks` (até aqui `common.c` pode ter um `printf`
  mínimo próprio).
- Diferencial com `gcc -m32` em `test/diff` (20–30 programas: ordenações, listas encadeadas,
  hash, parser de expressões, matriz, strings).
- `hello`/`blink`/`selftest` em simavr e QEMU via `avr-custom`/`arm-custom`.
- Documentação de uso (`README`), `kscc --help`.

Critério: **v1 pronta**. Um programa C com `malloc`, `printf`, structs e ponteiros de função
roda no desktop e no AVR.

### Fase 5 — `long long` (64 bits)
- Par de registradores `r1:r2` para valores de 64 bits no acumulador; `push`/`pop` duplos;
  `add`/`sub` com `adc`/`sbc`; `mul` com `mulhu` + produtos cruzados; `div`/`rem` por chamada a
  `__ks_divdi3` etc. na `libks` (em C com `unsigned` de 32 bits, estilo libgcc); shifts por
  rotina; comparações em dois passos.
- `int64_t`, `%lld`.

### Fase 6 — Ponto flutuante
- Depende da decisão na ksvm2: extensão F (opcodes soft-float na VM) ou biblioteca
  `libfloat`. O codegen chama `__ks_addsf3`, `__ks_mulsf3`, … em qualquer caso; a diferença é
  se essas rotinas são `.ksasm` de 3 linhas (opcodes F) ou C/asm de centenas de linhas.
- `float`/`double` (`double` = IEEE binary64 via rotinas de 64 bits) e `%f` no `printf`.

### Fase 7 — Qualidade de código
Medir com `INSN_COUNT` nos benchmarks antes de cada item:
1. Imediatos em `addi`/`subi`/`andi`/`ori`/`xori`/`shli`/`shri`/`sari`/`cmpi` quando `rhs` é constante (elimina `push`/`pop` + `movi r2`).
2. Comparação fundida no desvio (`if (a < b)` sem materializar 0/1).
3. Load/store com offset direto (`ldw r1, [fp + off]`) em vez de `addi` + `ldw`.
4. Alocação de registradores simples: variáveis locais escalares cujo endereço nunca é tomado
   vão para `r16`–`r29` (callee-saved); prólogo salva só os usados.
5. `switch` denso → tabela de saltos com `jmpr`.
6. Eliminação de código morto no "linker" (grafo de chamadas sobre o texto).
7. Peephole sobre o `.ksasm` (`push r1; pop r2` → `mov r2, r1`, etc.).

## 2. Estimativas

| Fase | Trabalho humano experiente | Com sub-agentes no modelo da ksvm2 |
|------|---------------------------|-------------------------------------|
| 0 | 1 dia | horas |
| 1 | 4–5 dias | 1 agente, 1 rodada |
| 2 | 5–7 dias | 1–2 agentes |
| 3 | 3–4 dias | 1 agente |
| 4 | 5–7 dias | 2 agentes em paralelo (libks, diferencial/embarcado) |
| **v1 (0–4)** | **~4 semanas** | **~3–4 rodadas + revisão** |
| 5 | 1 semana | 1 agente |
| 6 | 1–2 semanas (depende da extensão F) | 1–2 agentes |
| 7 | incremental | 1 agente por item |

Tamanho final estimado: `codegen.c` ~1.400 linhas (sem float), `libks` ~1.700 linhas de C +
150 de asm, testes adaptados ~3.000 linhas, testes próprios ~1.500.

## 3. Riscos

| Risco | Impacto | Mitigação |
|-------|---------|-----------|
| O front-end do chibicc assume `long` de 64 bits em `eval()` e nas conversões usuais; mudar tamanhos quebra constant folding de forma sutil | Resultados errados em expressões constantes | Fase 1 inclui testes de constantes nas bordas (`0x7FFFFFFF + 1`, `-1u`, shifts de 31); o diferencial com `gcc -m32` na fase 4 pega o resto |
| `ptrdiff_t`/`size_t` internos como `long` no parser | `sizeof` e aritmética de ponteiro com 8 bytes | Trocar `ty_long` por `ty_int`/`ty_uint` nos pontos de `pointer_diff` e `sizeof` (três lugares em `parse.c`) |
| Programas de teste muito grandes para o `ksas` | Falha de montagem | Pré-requisito de limites configuráveis |
| Desempenho ruim da v1 (push/pop em tudo) | Benchmarks lentos; não bloqueia correção | Fase 7; a VM faz ≥ 50 M instruções/s, então mesmo código ingênuo é utilizável |
| `alloca`/VLA mexem em `sp` com temporários empilhados | Bugs sutis de pilha | Portar fielmente `builtin_alloca` do chibicc, que já resolve isso movendo os temporários; teste `alloca.c` e `vla.c` |
| Fase 6 depende de decisão na ksvm2 | Bloqueio | Codegen chama rotinas `__ks_*sf3` desde já; a implementação pode ser trocada sem mexer no compilador |
| Divergir demais do chibicc e perder a capacidade de comparar | Manutenção mais cara | Manter nomes e estrutura de funções; `tools/diff_upstream.sh` na revisão de cada fase |

## 4. Estrutura do repositório `kscc`

```
kscc/
├── LICENSE              MIT (Rui Ueyama + fork)
├── README.md
├── docs/                (este diretório)
├── src/                 chibicc.h main.c tokenize.c preprocess.c parse.c type.c codegen.c
│                        strings.c hashmap.c unicode.c  +  link.c (concatenação) target.h (ABI)
├── include/kscc/        stddef.h stdarg.h stdbool.h stdint.h limits.h …
├── libks/
│   ├── asm/             crt0.ksasm syscall.ksasm abort.ksasm
│   ├── include/         string.h stdio.h stdlib.h … ksvm/*.h
│   ├── src/             *.c
│   └── Makefile
├── test/
│   ├── unit/            um .c por construção, com "// expect:"
│   ├── chibicc/         suite adaptada + common.c
│   ├── diff/            programas para comparação com gcc -m32
│   ├── bench/           fib sieve qsort strlen
│   ├── embedded/        hello blink selftest
│   └── run.sh           harness
├── tools/               diff_upstream.sh
└── Makefile             KSVM2_HOME=../ksvm2
```
