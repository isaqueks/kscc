# 02 — ABI do kscc para a KSVM v2

Esta ABI é a convenção que o doc 04 da ksvm2 sugere, fechada nos detalhes que um compilador
precisa. A `libks` e qualquer código `.ksasm` chamado de C devem segui-la.

## 1. Modelo de dados (ILP32, little-endian)

| Tipo C | Tamanho | Alinhamento | Registrador |
|--------|--------:|------------:|-------------|
| `_Bool`, `char`, `signed char`, `unsigned char` | 1 | 1 | estendido para 32 bits ao ser carregado (`ldbs`/`ldb`) |
| `short`, `unsigned short` | 2 | 2 | idem (`ldhs`/`ldh`) |
| `int`, `unsigned`, `long`, `unsigned long`, enum, ponteiro, `size_t`, `ptrdiff_t`, `intptr_t` | 4 | 4 | um registrador |
| `long long`, `unsigned long long` | 8 | 4 | par de registradores (fase 5) — **não suportado na v1: erro de compilação** |
| `float` / `double` / `long double` | 4 / 8 / 8 | 4 | fase 6 — **erro na v1** |
| struct/union | soma dos membros | maior alinhamento dos membros (máx. 4) | sempre em memória |

`char` é **signed** (como x86). Alinhamento máximo natural é 4; `_Alignas(8)` ou maior é aceito
e honrado só em globais e na pilha (ajuste de `sp`), nunca necessário para a VM.

## 2. Registradores

| Registrador | Papel no kscc | Quem preserva |
|-------------|---------------|---------------|
| `zr` (r0) | constante 0 | — |
| `r1` | acumulador; argumento 1; valor de retorno | chamador |
| `r2` | segundo operando de binários; argumento 2 | chamador |
| `r3`, `r4` | temporários; argumentos 3 e 4 | chamador |
| `r5` | temporário: endereço de função em chamadas indiretas, contador de cópias | chamador |
| `r6`–`r15` | temporários livres (v1 usa só `r6`–`r8` em cópias de struct e bitfields) | chamador |
| `r16`–`r29` | reservados para alocação de registradores futura (fase 7) | chamado |
| `fp` (r30) | frame pointer | chamado |
| `sp` (r31) | stack pointer, múltiplo de 4 | chamado (restaurado no epílogo) |

Na v1 o codegen não usa `r16`–`r29`, então nenhuma função precisa salvá-los. Quando a fase 7
começar a usá-los, o prólogo salva só os que a função toca.

## 3. Convenção de chamada

### 3.1 Funções com protótipo fixo
1. O chamador avalia os argumentos **da direita para a esquerda**, empilhando cada um (`push`).
   Structs/unions são copiadas para a pilha inteiras, arredondadas para múltiplo de 4.
2. Depois de avaliar todos, os quatro primeiros argumentos **escalares** são retirados da pilha
   para `r1`–`r4` (`pop r1`, `pop r2`, …). Argumentos de struct e os escalares a partir do 5º
   ficam na pilha, na ordem em que o chamado os encontrará: argumento mais à esquerda no endereço
   mais baixo.
3. `call f` (ou `callr r5`).
4. Ao retornar, o chamador libera os argumentos que ficaram na pilha: `addi sp, sp, N`.

Isso é exatamente o `push_args` do chibicc (duas passadas: primeiro os que vão para a pilha,
depois os que vão para registradores), só que com 4 registradores em vez de 6 e sem a classe
de ponto flutuante.

### 3.2 Funções variádicas
Todos os argumentos, inclusive os nomeados, vão para a pilha. O chamado sabe que é variádica
pelo seu próprio protótipo; o chamador sabe pelo protótipo visível (C exige protótipo para
chamar variádicas, e o chibicc já rejeita a falta dele). `va_list` é um `char*` que aponta para
o primeiro argumento anônimo em `fp + 8 + (tamanho dos nomeados)`; `va_arg(ap, T)` lê 4 bytes
(ou `sizeof(T)` arredondado a 4) e avança. Promoções padrão (`char`→`int`) já são do C.

### 3.3 Valor de retorno
- Escalares: `r1`.
- Struct/union de **até 4 bytes**: `r1` (bytes baixos).
- Struct/union maior: o chamador reserva espaço no próprio frame e passa o endereço como
  **argumento oculto em `r1`**, deslocando os demais (`r2`–`r4`, pilha). O chamado copia o
  resultado para lá e devolve o mesmo endereço em `r1`.

### 3.4 Flags
Nunca preservadas através de chamadas. O codegen nunca depende de flags sobreviverem a um
`call`, `push`, `pop` ou `mov` (estes não alteram flags, mas a regra é não depender).

## 4. Frame de pilha

```
endereços altos
        ┌──────────────────────────┐
        │ argumento n (na pilha)   │  fp + 8 + 4(k)   (k = índice entre os args na pilha)
        │ …                        │
        │ argumento 5 / 1º struct  │  fp + 8
        ├──────────────────────────┤
        │ endereço de retorno      │  fp + 4          (empilhado pelo CALL)
        ├──────────────────────────┤
        │ fp do chamador           │  fp + 0          (push fp no prólogo)
        ├──────────────────────────┤
        │ área de varargs (se var.)│  —  (não há: os args já estão em fp+8)
        │ cópia dos args em r1–r4  │  fp − 4 … fp − 16   (só os que a função usa como lvalue;
        │                          │   na v1, sempre copiados: simples e correto)
        │ variáveis locais         │  fp − off (off cresce para baixo, alinhado ao tipo)
        │ buffer de retorno struct │
        ├──────────────────────────┤
        │ temporários de expressão │  push/pop durante gen_expr
        │ (alloca / VLA)           │  abaixo dos temporários, sp ajustado dinamicamente
        └──────────────────────────┘  sp
endereços baixos
```

Prólogo:
```asm
f:
        push    fp
        mov     fp, sp
        subi    sp, sp, FRAME        ; FRAME múltiplo de 4; se > 8191: addl sp, sp, -FRAME
        stw     [fp - 4],  r1        ; argumentos recebidos em registrador → slots locais
        stw     [fp - 8],  r2
        ...
```
Epílogo (único por função; `return` salta para `L_return_f`):
```asm
L_return_f:
        mov     sp, fp
        pop     fp
        ret
```

Verificação de estouro: a VM já gera `FAULT_STACK` se `sp` cruzar `stack_limit`
(`KSVM_CHECK_STACK`). O compilador não emite checagem própria.

## 5. Símbolos e seções

| Objeto C | Seção `ksas` | Nome no assembly |
|----------|--------------|------------------|
| função ou global externa | `.text` / `.data` / `.bss` | nome C inalterado, com `.global` |
| função ou global `static` | idem | `_S<id>_nome`, onde `<id>` é único por unidade de tradução (hash curto do caminho) |
| string literal, literal composto global | `.rodata` | `_L<n>_str` |
| variável local `static` | `.data`/`.bss` | `_L<n>_nome` |
| label de controle (`.L.else.3` no chibicc) | `.text` | `L<id>.else.3` (`<id>` = hash da unidade; o `ksas` trata nomes iniciados por `.` como diretiva) |
| `main` | `.text` | `main`, chamado por `_start` do `crt0` |

Globais inicializadas com endereços usam `.word sym + off`, que o `ksas` já resolve.
`.bss` usa `.space n`. Dados `const` vão para `.rodata` (região CODE, só leitura: escrever em
`const` dá `FAULT_MEM_PERM` em vez de corromper silenciosamente).

## 6. Interoperação com `.ksasm` escrito à mão

Uma função em assembly chamável de C precisa: aceitar `r1`–`r4` e pilha conforme §3, preservar
`fp`/`sp` (e `r16`–`r29` quando a fase 7 chegar), devolver em `r1`, terminar em `ret`.
A `libks` usa isso para os wrappers de syscall (doc 05).
