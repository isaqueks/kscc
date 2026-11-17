# 04 — Driver, "linker" e integração com a toolchain da ksvm2

## 1. Linha de comando

```
kscc [opções] arquivo.c ...          # compila, monta e "linka" → a.ksx
  -S                 só o assembly .ksasm
  -c                 compila + monta uma unidade → .kso (ver §3)
  -E                 só o preprocessador
  -o <saida>
  -I <dir>  -D NAME[=val]  -U NAME  -include <arq>
  -nostdlib          não acrescenta crt0 nem libks
  -L <dir>  -l<nome> bibliotecas .kso (libks fica em <prefixo>/lib/libks.kso)
  -g                 passa -g ao ksas (tabela de símbolos completa)
  -fverbose-asm      comentários com a linha C ao lado de cada bloco
  --ksas <caminho>   --ksvm <caminho>   (padrão: KSVM2_HOME ou PATH)
  -###               mostra os comandos sem executar
```
Mantém a estrutura `run_cc1` do chibicc (o driver re-invoca a si mesmo com `-cc1` por arquivo),
porque isso isola estado global do parser entre unidades de tradução.

## 2. Pipeline

```
a.c ──cc1──► a.ksasm ──ksas──► a.kso ──┐
b.c ──cc1──► b.ksasm ──ksas──► b.kso ──┼──link──► prog.ksasm ──ksas──► prog.ksx ──ksvm──► execução
crt0.ksasm, libks.kso ─────────────────┘
```

## 3. O que é um `.kso` e por que não há linker binário

O `.ksx` da ksvm2 é uma imagem final, sem relocações nem tabela de símbolos externos. Escrever
um formato de objeto relocável e um linker custaria mais que o resto do kscc. Em vez disso:

- **`.kso` é o próprio texto `.ksasm` da unidade**, já validado sintaticamente pelo `ksas`
  (`ksas -c` monta em memória só para acusar erros e descarta o binário). Extensão diferente para
  deixar claro que é saída de compilador, não fonte editado à mão.
- **Link = concatenação ordenada** em um único `.ksasm`:
  1. `crt0.ksasm` (define `_start`, chama `main`, faz `EXIT`);
  2. os `.kso` do usuário na ordem da linha de comando;
  3. `libks.kso` (uma unidade só, gerada por `cat` dos `.kso` da biblioteca);
  4. um rodapé com `.entry _start` e `.stack N` (`-Wl,--stack=N`, padrão 4096).
- Cada `.kso` começa com `.text` e termina em estado neutro, então a ordem de seções no arquivo
  final é cuidada pelo `ksas` (que já coloca `.rodata` depois de `.text` e `.data`/`.bss` na RAM).
- **Símbolos**: externos são nomes C puros (`.global`); `static` levam prefixo `_S<id>_`
  (doc 02 §5), então nunca colidem entre unidades. Uma função definida duas vezes é acusada pelo
  `ksas` como label duplicado, com os dois arquivos de origem visíveis pelos comentários `; file`.
- **Símbolos indefinidos**: o `ksas` acusa "símbolo não definido" com a linha. O driver traduz
  a mensagem para "referência indefinida a `f`" procurando a declaração no `.kso`.
- **Código morto**: nada é removido (sem `--gc-sections`). A `libks` é pequena; se o tamanho
  importar para o AVR, a fase 7 pode fazer eliminação por grafo de chamadas sobre o texto.

Limites: a concatenação cria uma única seção `.text` de até 16 MiB (limite do `ksas`). Muito
além de qualquer programa de teste; para um projeto grande seria a hora de um linker real.

## 4. Macros predefinidas

```
__ksvm__ 1        __ksvm_isa__ 0x0200     __ILP32__ 1
__CHAR_BIT__ 8    __SIZEOF_INT__ 4  __SIZEOF_LONG__ 4  __SIZEOF_POINTER__ 4
__SIZEOF_SHORT__ 2  __SIZEOF_LONG_LONG__ 8 (fase 5)
__STDC__ 1  __STDC_VERSION__ 201112L  __STDC_HOSTED__ 1 (libks) ou 0 (-ffreestanding)
__kscc__ 1  __kscc_version__ "0.1"
__BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
```
Remover `__x86_64__`, `__linux__`, `__gnu_linux__`, `__LP64__` que o chibicc define.

## 5. Caminhos de include

1. `-I` da linha de comando;
2. `<prefixo>/include` (headers da `libks`, doc 05);
3. `<prefixo>/include/kscc` (headers que o próprio compilador fornece: `stddef.h`, `stdarg.h`,
   `stdbool.h`, `stdint.h`, `limits.h`, `stdalign.h`, `stdnoreturn.h`, `float.h` só na fase 6).

Nunca `/usr/include`: os headers do host assumem LP64 e glibc.

## 6. Execução e depuração

```
kscc hello.c -o hello.ksx && ksvm hello.ksx           # roda
kscc -S hello.c -o -                                  # vê o assembly
ksvm --trace hello.ksx 2> trace.txt                   # instrução a instrução
kscc -g hello.c -o hello.ksx && ksdis hello.ksx       # desassembla com nomes de função
```
Erros de execução aparecem como `fault: NOME at pc=0x… addr=0x…`; com `-g` o `ksdis` mostra em
que função o `pc` cai.

## 7. Integração com as portas embarcadas

`kscc -o app.ksx app.c` e depois, na ksvm2, `make avr-custom KSX=app.ksx` ou
`make arm-custom KSX=app.ksx`. A `libks` usa só syscalls dos grupos 0, 1, 2, 5 (console,
tempo, memória), todos implementados em `hal_avr` e `hal_arm_cm`, então um programa C que só
usa `printf`/`puts`/`getchar`/`millis` roda nos três lugares sem mudar.

Para I/O de hardware em C: `#include <ksvm/syscall.h>` da `libks` expõe `ks_pin_mode`,
`ks_pin_write`, `ks_pin_read`, `ks_analog_read` (grupo 4) como funções inline em assembly.
