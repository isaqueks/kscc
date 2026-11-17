# 01 — Visão, escopo e por que chibicc

## 1. Objetivo

Compilar programas em C (C11, sem ponto flutuante na v1) para a KSVM v2, de modo que o mesmo
programa rode sem alteração no desktop (`ksvm` runner), no ATmega328P e no Cortex-M através
das portas já existentes da ksvm2.

Critério de sucesso da v1: a suite de testes do chibicc (menos os arquivos de float, TLS,
atomics e asm inline) passa inteira dentro do `ksvm`, e a `libks` (escrita em C) é compilada
pelo próprio `kscc`.

## 2. Por que fork do chibicc

| Propriedade do chibicc | Consequência para o kscc |
|------------------------|--------------------------|
| 8.900 linhas de C11 limpo, um arquivo por fase (`tokenize`, `preprocess`, `parse`, `type`, `codegen`) | O trabalho se concentra em 1 arquivo; o resto é lido, não reescrito |
| Suporta quase todo C11: structs/unions, bitfields, enums, typedef, VLA, `_Generic`, `_Alignas`, literais compostos, designated initializers, varargs, preprocessador completo com `#include`, macros variádicas, `__VA_OPT__` não, mas `#pragma once` sim | O kscc herda tudo isso no dia 1 |
| Codegen "stack machine com acumulador": `gen_expr` deixa o valor em `rax`, `push`/`pop` para operandos, locais em `off(%rbp)` | Mapeia 1:1 para `r1`, `PUSH`/`POP`, `[fp + off]` |
| Emite assembly em texto, chama `as` e `ld` externos | Trocamos por `ksas`; nenhum formato de objeto binário para inventar |
| Suite de 49 arquivos de teste + `driver.sh` | Oráculo pronto; só precisa de ajustes de ILP32 |
| Licença MIT | Fork com atribuição; sem restrição de uso |
| Sem dependências além da libc do host | Compila com gcc/clang no WSL como a ksvm2 |

Alternativas descartadas: `lcc` (C89, licença não padrão), `tcc` (codegen entrelaçado com o
parser, difícil de retargetar), `LLVM`/`GCC` (meses de trabalho; podem vir depois se o objetivo
virar desempenho).

## 3. O que muda no chibicc

| Arquivo | Linhas | Mudança |
|---------|-------:|---------|
| `codegen.c` | 1.595 | **Reescrito** para a KSVM (doc 03). Mantém a estrutura e os nomes de função para facilitar comparação com o original |
| `type.c` | 307 | Tamanhos: `long` 4, ponteiro 4, `long long` 8 (fase 5), `float/double` 4/8 (fase 6, mas tipos existem desde já para o parser aceitar declarações) |
| `parse.c` | 3.368 | Constant folding (`eval`) e conversões usuais passam a respeitar 32 bits; `ptrdiff_t`/`size_t` viram `int`/`unsigned`; `__builtin_*` reduzidos |
| `main.c` | 791 | Driver: `-S`, `-c`, `-o`, `-I`, `-D`, `-E`; `assemble()` chama `ksas`; `run_linker()` vira a concatenação do doc 04 |
| `preprocess.c`, `tokenize.c` | 2.013 | Macros predefinidas (`__ksvm__`, `__ILP32__`, tamanhos) e remoção de `__x86_64__` |
| `include/` | — | Headers próprios da `libks` (doc 05) no lugar dos headers do sistema |
| `test/` | — | Fork da suite com valores ILP32 e execução no `ksvm` (doc 06) |

## 4. Escopo da v1 (fases 0–4 do doc 07)

Dentro:
- Tipos inteiros de 8/16/32 bits com e sem sinal, `_Bool`, enums, ponteiros, arrays, structs,
  unions, bitfields, `typedef`, `static`, `extern`, `const`/`volatile` (aceitos, sem efeito).
- Todas as expressões e comandos do C11, `switch`, `goto`, labels, `? :`, operador vírgula,
  statement expressions (extensão GNU que o chibicc já aceita).
- Funções com até N argumentos, recursão, ponteiros de função, varargs (`<stdarg.h>`).
- Inicializadores de globais com endereços (`.word sym+off`), strings, arrays de structs.
- Preprocessador completo.
- `alloca` e VLA (ambos manipulam `sp`; ficam como "opcional" na fase 3).

Fora da v1 (fases posteriores): `long long`, `float`/`double`/`long double`, `_Thread_local`,
`_Atomic`, `asm` inline (poderia virar `asm("...")` → linha `.ksasm` literal; decisão na fase 7),
`__attribute__((packed))` (o chibicc suporta; manter se for barato).

## 5. Restrições que a KSVM impõe e que o compilador tem de respeitar

- Offsets imediatos de load/store e `addi` têm 14 bits com sinal (−8192..8191). Frames ou
  structs maiores precisam de endereço calculado em registrador (`addl`).
- Desvios `Bcc` alcançam ±2 MiB e `jmp`/`call` ±32 MiB; o `ksas` limita cada seção a 16 MiB.
  Nenhum programa de teste chega perto; o codegen pode assumir alcance suficiente e deixar o
  `ksas` acusar erro.
- Sem instrução `SETcc`: comparações que produzem valor usam `movi` + desvio condicional.
- `sp` deve permanecer múltiplo de 4. Todo `push` é de 32 bits; `char` e `short` são
  promovidos ao serem empilhados.
- O registrador `zr` é 0 constante e serve para zerar memória (`stw [fp+off], zr`) e para
  comparar com zero (`cmp r1, zr`).

## 6. Licença e atribuição

O repositório `kscc` mantém o `LICENSE` MIT original com o copyright de Rui Ueyama e adiciona
o do fork. Cada arquivo derivado conserva o cabeçalho de origem. O `README` deixa explícito que é
um fork do chibicc retargetado para a KSVM.
