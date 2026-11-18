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
  -Wl,--stack=N      tamanho da pilha no cabeçalho .ksx (padrão 4096)
  -Wl,--no-gc-sections  liga a libks inteira (padrão: só o que é referenciado)
  (libks fica em build/libks/libks.kso ao lado do kscc que a compilou; $KSCC_LIBDIR troca)
  -g                 passa -g ao ksas (tabela de símbolos completa)
  --ksas <caminho>   assembler a usar (padrão: KSVM2_HOME/build/ksas)
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
- **Código morto** (implementado na fase 4): cada label global da `libks.kso` delimita um bloco;
  o link inclui só os blocos alcançáveis a partir do código do usuário (referências em
  `call`/`la`/`.word`/etc.). Uma definição do usuário vence a da libks. `hello.c` com `printf`
  cai de 43 KB para 15 KB. `-Wl,--no-gc-sections` desliga.
- **Referência fraca** (fase 6): um símbolo comum da libks com nome `__ks_weak_*` (definição
  provisória numa unidade, inicializada em outra) resolve para o seu marcador comum, um `.bss`
  zerado. A definição inicializada só entra se esse marcador entrou no link **e** alguma unidade
  do usuário a pede com a linha `; kscc-need: __ks_weak_nome` (comentário para o `ksas`). O
  codegen emite `; kscc-need: __ks_weak_printf_float` numa unidade que passa `float`/`double`
  como argumento anônimo de uma função variádica: assim a conversão de ponto flutuante do
  `printf` (~20 KB, doc 05 §2) só é ligada quando o programa usa `printf` **e** passa floats a
  variádicas. Sem ela, `%f` consome o argumento e imprime `<?>`.

Limites: a concatenação cria uma única seção `.text` de até 16 MiB (limite do `ksas`). Muito
além de qualquer programa de teste; para um projeto grande seria a hora de um linker real.

## 4. Macros predefinidas

```
__ksvm__ 1        __ksvm_isa__ 0x0200     __ILP32__ 1
__CHAR_BIT__ 8    __SIZEOF_INT__ 4  __SIZEOF_LONG__ 4  __SIZEOF_POINTER__ 4
__SIZEOF_SHORT__ 2  __SIZEOF_LONG_LONG__ 8 (fase 5)
__SIZEOF_FLOAT__ 4  __SIZEOF_DOUBLE__ 4  __SIZEOF_LONG_DOUBLE__ 4   (todos binary32, doc 02 §1.1)
__FLT_EVAL_METHOD__ 0   __kscc_float__ 1
__STDC__ 1  __STDC_VERSION__ 201112L  __STDC_HOSTED__ 1 (libks) ou 0 (-ffreestanding)
__kscc__ 1  __kscc_version__ "0.1"
__BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
```
Remover `__x86_64__`, `__linux__`, `__gnu_linux__`, `__LP64__` que o chibicc define.
`__STDC_IEC_559__` não é definida: `double` não é binary64.

## 5. Caminhos de include

1. `-I` da linha de comando;
2. `<prefixo>/include` (headers da `libks`, doc 05);
3. `<prefixo>/include/kscc` (headers que o próprio compilador fornece: `stddef.h`, `stdarg.h`,
   `stdbool.h`, `stdint.h`, `limits.h`, `stdalign.h`, `stdnoreturn.h`, `float.h`, com `DBL_*`
   e `LDBL_*` iguais aos `FLT_*`).

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
`make arm-custom KSX=app.ksx`. Ponto flutuante usa a extensão F da VM, presente no desktop e no
Cortex-M e **ausente na porta AVR** (`CAPS(0x4000)` bit 2; uma instrução F lá dá
`FAULT_BAD_OPCODE`): um programa que pode rodar nos dois deve consultar `ks_sys_caps(0x4000) & 4`
antes de calcular com float (`test/embedded/fselftest.c`). `printf("%f")` e `strtof` não usam
instruções F e funcionam também no AVR. A `libks` usa só syscalls dos grupos 0, 1, 2, 5 (console,
tempo, memória), todos implementados em `hal_avr` e `hal_arm_cm`, então um programa C que só
usa `printf`/`puts`/`getchar`/`millis` roda nos três lugares sem mudar.

Para I/O de hardware em C: `#include <ksvm/syscall.h>` da `libks` expõe `ks_pin_mode`,
`ks_pin_write`, `ks_pin_read`, `ks_analog_read` (grupo 4) como funções inline em assembly.
