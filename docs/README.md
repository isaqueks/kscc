# kscc — Compilador C para a KSVM v2: plano de projeto

`kscc` é um compilador de C para a máquina virtual KSVM v2, obtido por **fork do
[chibicc](https://github.com/rui314/chibicc)** (Rui Ueyama, licença MIT, ~8.900 linhas de C) com
o gerador de código reescrito para emitir assembly `.ksasm` do `ksas`.

Projeto independente da `ksvm2`: repositório próprio, depende dela apenas como ferramenta
(`ksas`, `ksvm`) e como alvo (ISA do `docs/04-isa.md` da ksvm2).

Nada aqui é código ainda. Estes documentos são a especificação que o fork deve seguir.

| # | Documento | Conteúdo |
|---|-----------|----------|
| 01 | [Visão, escopo e por que chibicc](01-visao-escopo.md) | Objetivo, o que o chibicc dá de graça, o que muda, o que fica fora, licença |
| 02 | [ABI](02-abi.md) | Modelo de dados ILP32, uso de registradores, convenção de chamada, frame de pilha, structs, varargs |
| 03 | [Gerador de código](03-codegen.md) | Mapeamento função a função de `codegen.c` (x86-64) para a KSVM, com o assembly exato |
| 04 | [Driver, "linker" e toolchain](04-driver-toolchain.md) | CLI, unidades de tradução, concatenação, símbolos locais, integração com `ksas`/`ksvm` |
| 05 | [Biblioteca de runtime `libks`](05-libks.md) | crt0, syscalls, string/stdio/stdlib mínimos, headers, malloc |
| 06 | [Testes](06-testes.md) | Adaptação da suite do chibicc para ILP32 e para rodar no `ksvm`, testes próprios, diferencial |
| 07 | [Roadmap e riscos](07-roadmap.md) | Fases, ordem, critérios de conclusão, pré-requisitos na ksvm2, estimativas, riscos |

## Decisões-chave (resumo executivo)

1. **Fork, não reescrita.** Front-end (tokenizer, preprocessador, parser, tipos) do chibicc fica
   quase intacto. Só `codegen.c` (1.595 linhas) é reescrito, mais ajustes pontuais em `type.c`
   (tamanhos), `parse.c` (constantes e conversões) e `main.c` (driver).
2. **ILP32**: `int`, `long` e ponteiros com 32 bits. `long long` (64 bits) e `float`/`double`
   ficam fora da v1 e entram em fases posteriores via rotinas de runtime.
3. **Codegen de acumulador + pilha**, igual ao chibicc: resultado em `r1`, operando esquerdo
   empilhado com `PUSH`/`POP`, `r2` como segundo operando, `r3`–`r5` como temporários. Sem
   alocação de registradores na v1. Correção primeiro, desempenho depois.
4. **Convenção de chamada**: `r1`–`r4` para os quatro primeiros argumentos inteiros/ponteiro,
   resto na pilha; retorno em `r1`; funções variádicas recebem **tudo** na pilha.
5. **Sem linker**: cada unidade de tradução vira um `.ksasm`; o "link" é a concatenação de
   `crt0 + objetos + libks` em um único `.ksasm` montado pelo `ksas`. Símbolos `static`
   recebem prefixo único por arquivo.
6. **Suite de testes do chibicc como oráculo**: os 49 arquivos de `test/` são adaptados para
   ILP32 e executados dentro do `ksvm`. Um teste que passa é uma feature comprovada.
7. **Nenhum pré-requisito bloqueante na ksvm2**: labels ganham prefixo por unidade (`L<id>.`),
   os limites do `ksas` bastam, e `ksvm` já devolve o código de saída do guest (doc 07 §0).
