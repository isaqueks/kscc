# 03 — Gerador de código: de `codegen.c` (x86-64) para a KSVM

Princípio: **mesma estrutura, mesmas funções, mesmos nomes**. Quem conhece o `codegen.c` do
chibicc reconhece o do kscc linha a linha; a diferença é só o texto emitido. Isso é o que torna
o porte barato e revisável.

Invariantes do chibicc que permanecem:
- `gen_expr(node)` deixa o valor em **`r1`** (era `rax`). Valores de `char`/`short` chegam
  sempre estendidos a 32 bits.
- `gen_addr(node)` deixa o **endereço** em `r1`.
- `push()` / `pop(reg)` empilham/desempilham 32 bits e mantêm `depth` para o chibicc saber quanto
  há na pilha (ele usa isso para alinhar chamadas; na KSVM só precisa ser múltiplo de 4, o que o
  `push` de 4 bytes já garante).
- Binários: `gen_expr(rhs); push(); gen_expr(lhs); pop("r2");` → `r1 op r2` (era `rax op rdi`).

## 1. Tabela de emissão por função

### `push` / `pop`
```asm
push r1                 ; depth++
pop  r2                 ; depth--
```

### `gen_addr`
| Caso | x86-64 | KSVM |
|------|--------|------|
| local (`offset` relativo a fp) | `lea off(%rbp), %rax` | `addi r1, fp, off` ; se fora de ±8191: `addl r1, fp, off` |
| global / função | `lea sym(%rip), %rax` | `movil r1, sym` (absoluto; com `-fpic` futuro: `adrl r1, sym`) |
| local `static` | idem global | `movil r1, _L<n>_nome` |
| `ND_DEREF` | `gen_expr(lhs)` | igual |
| `ND_MEMBER` | `gen_addr(lhs); add $off, %rax` | `gen_addr(lhs); addi r1, r1, off` (ou `addl`) |
| `ND_COMMA`, `ND_FUNCALL` (retorno struct), `ND_VLA_PTR` | | iguais, com registradores trocados |

### `load(ty)`
| Tipo | KSVM |
|------|------|
| array, struct, union, função, VLA | nada (o valor é o endereço) |
| 1 byte signed / unsigned | `ldbs r1, [r1]` / `ldb r1, [r1]` |
| 2 bytes | `ldhs r1, [r1]` / `ldh r1, [r1]` |
| 4 bytes | `ldw r1, [r1]` |

### `store(ty)` — valor em `r1`, endereço empilhado
```asm
pop r2
; struct/union de N bytes: cópia word a word (resto em bytes), usando r3 como temporário
ldw r3, [r1 + 0]  / stw [r2 + 0], r3 ... ; para N <= 32 bytes desenrolado; senão laço com r5 contador
; escalares:
stb [r2], r1   |   sth [r2], r1   |   stw [r2], r1
```
Offsets da cópia desenrolada passam de 8191 só para structs gigantes; nesse caso o laço é
obrigatório. Regra: desenrolar até 32 bytes, laço acima.

### `cmp_zero(ty)` e condições
x86 usa `cmp $0, %eax` + `je`. KSVM:
```asm
cmp r1, zr              ; flags de r1 - 0: Z = (r1 == 0)
beq L                   ; "je"
```
Como `cmp` é a única fonte de flags e `movi`/`mov`/`push`/`pop` não as alteram, as sequências
`cmp` → `movi` → `bcc` do chibicc continuam corretas.

### Comparações que produzem valor (`==`, `!=`, `<`, …) — não há `SETcc`
```asm
cmp  r1, r2             ; lhs em r1, rhs em r2
movi r1, 1
b<cc> L_set_N           ; cc = eq/ne/lt/le/gt/ge (signed) ou lo/ls/hi/hs (unsigned)
movi r1, 0
L_set_N:
```
Escolha de `cc` sem sinal quando `lhs->ty->is_unsigned` (o chibicc já faz essa distinção com
`setb`/`setbe` vs `setl`/`setle`). Otimização óbvia para a fase 7: quando a comparação é usada
diretamente em `if`/`while`/`for`/`?:`/`&&`/`||`, fundir no desvio e não materializar 0/1.

### `cast(from, to)`
A tabela `cast_table[][]` do chibicc (10×10 de strings) vira uma tabela sobre 5 classes:
`i8, i16, i32, u8, u16, u32, bool` (float/long long entram nas fases 5–6).

| destino | KSVM |
|---------|------|
| `i8` | `sxb r1, r1` |
| `u8` | `zxb r1, r1` |
| `i16` | `sxh r1, r1` |
| `u16` | `zxh r1, r1` |
| `i32`/`u32`/ponteiro | nada |
| `bool` | `cmp r1, zr; movi r1, 1; bne L; movi r1, 0; L:` |

### Aritmética (`r1 = r1 op r2`)
| C | KSVM | Observação |
|---|------|------------|
| `+` `-` | `add r1, r1, r2` / `sub r1, r1, r2` | aritmética de ponteiro: o parser já multiplica pelo tamanho |
| `*` | `mul r1, r1, r2` | |
| `/` signed/unsigned | `div r1, r1, r2` / `divu r1, r1, r2` | divisão por zero → `FAULT_DIV_ZERO` (comportamento definido pela VM, não UB) |
| `%` | `rem` / `remu` | |
| `&` `\|` `^` | `and` / `or` / `xor` | |
| `<<` | `shl r1, r1, r2` | |
| `>>` | `sar` (signed) / `shr` (unsigned) | |
| unário `-` | `neg r1, r1` | |
| `~` | `not r1, r1` | |
| `!` | `cmp r1, zr; movi r1, 1; beq L; movi r1, 0; L:` | |
| `&&` `\|\|` | mesma estrutura de labels do chibicc com `cmp r1, zr` + `beq`/`bne` | |

Constantes como segundo operando: na v1 **sempre** via `r2` (`movi`/`movil r2, imm` após o
`pop` ser evitado). Fase 7 usa `addi`/`andi`/`shli` etc. quando `rhs` é `ND_NUM` que cabe em
14 bits.

### `ND_NUM`
`movi r1, v` se −8192 ≤ v ≤ 8191, senão `movil r1, v`. (O `ksas` faz essa escolha sozinho com
`li r1, v`; o codegen emite `li` e deixa o assembler decidir.)

### `ND_ASSIGN` e bitfields
Igual ao chibicc, com 32 no lugar de 64 nos shifts:
```asm
; leitura de bitfield (width w, offset o) após load:
shli r1, r1, 32 - w - o
shri r1, r1, 32 - w        ; ou sari se o tipo for signed
; escrita: r1 = valor novo; endereço em [sp]
mov  r6, r1                ; guarda valor para o resultado da expressão
andl r2, r1, (1<<w)-1
shli r2, r2, o
ldw  r1, [sp]              ; lê endereço
load(ty)                   ; valor atual
andl r1, r1, ~mask
or   r1, r1, r2
store(ty)
mov  r1, r6
```
(`ldw r1, [sp]` lê o topo sem desempilhar — equivalente ao `mov (%rsp), %rax`.)

### `ND_MEMZERO` (zerar local de N bytes)
x86 usa `rep stosb`. KSVM, com o registrador zero:
```asm
stw [fp + off + 0], zr
stw [fp + off + 4], zr     ; … desenrolado até 64 bytes; acima: laço com r2 = endereço, r3 = contador
stb [fp + off + k], zr     ; resto
```

### `ND_COND`, `ND_NOT`, `ND_BITNOT`, `ND_LOGAND`, `ND_LOGOR`
Estrutura idêntica; labels `L<id>.else.N`, `L<id>.end.N`, `L<id>.true.N`, `L<id>.false.N` (prefixo por unidade, doc 02 §5).

### `ND_FUNCALL`
```asm
; 1. gen_expr(fn) se chamada indireta → r1; push
; 2. push_args(): avalia e empilha (ordem do chibicc), inclusive structs (push_struct copia word a word)
; 3. se retorno é struct > 4 bytes: addi r1, fp, off_retbuf ; push   (argumento oculto)
; 4. pop r1 / pop r2 / pop r3 / pop r4 (só os escalares que couberam; variádica: nenhum)
; 5. chamada:
call  f                    ; direta
; ou
pop   r5                   ; endereço da função (empilhado no passo 1 antes dos args)
callr r5
; 6. addi sp, sp, bytes_na_pilha     (se houver)
; 7. retorno de char/short/bool: o chamado já estende; nada a fazer (chibicc re-estende; manter por segurança: sxb/zxb)
```
Detalhe: o chibicc empilha `fn` **antes** dos argumentos e faz `pop %r10` depois de distribuir
os registradores; o kscc usa `r5` no lugar de `r10`.

### Prólogo e epílogo (`emit_text`)
Ver doc 02 §4. `assign_lvar_offsets` muda só o alinhamento máximo (4) e o cálculo do
deslocamento dos parâmetros que ficaram na pilha (`fp + 8 + …`) em vez da convenção System V.
Funções variádicas: `__va_area__` do chibicc deixa de existir; `va_start` vira
`addi ap, fp, 8 + bytes_dos_nomeados` (o parser do chibicc expande `va_start` como builtin;
a expansão muda).

### `emit_data`
| chibicc | kscc (`ksas`) |
|---------|---------------|
| `.data` / `.bss` / `.section .rodata` | `.data` / `.bss` / `.rodata` |
| `.globl sym` | `.global sym` |
| `.align n` | `.align n` |
| `.byte v` | `.byte v` |
| `.quad sym+off` (ponteiro) | `.word sym + off` |
| `.zero n` | `.space n` |
| string | `.byte` por byte (o chibicc já emite assim, com escapes) ou `.asciz` quando puro ASCII |

### Diretivas de debug
`.loc file line` → comentário `; file:line` (o `ksas` ignora; útil no listing e no trace).
`.file` → comentário no topo.

## 2. Casos que precisam de atenção especial

| Situação | Tratamento |
|----------|------------|
| Offset de local/membro fora de ±8191 | `gen_addr` emite `addl`; `load`/`store` sempre usam `[r1]` com offset 0, então nunca estouram |
| `switch` | chibicc emite cadeia de `cmp`/`je` por `case` (e `case` com ranges GNU: dois `cmp`). Manter. Tabela de saltos com `jmpr` é otimização da fase 7 |
| `goto`, labels do usuário | `unique_label()` do chibicc com o prefixo `L<id>..N` |
| `alloca(n)` / VLA | `sub sp, sp, n` com `n` arredondado a 4; ponteiro = `sp`; liberação no epílogo (`mov sp, fp`), igual ao chibicc. O chibicc move os temporários da pilha; replicar a lógica de `builtin_alloca` com `r2`/`r3` |
| Statement expressions `({ … })` | inalterado |
| `_Generic`, `typeof`, `offsetof` | resolvidos no parser; sem codegen |
| Literais compostos locais | `ND_MEMZERO` + stores; inalterado |
| Globais `static` com o mesmo nome em arquivos diferentes | prefixo `_S<id>_` (doc 02 §5) aplicado em `emit_data`/`emit_text` e em `gen_addr` |
| `long long` (v1) | `error_tok("long long nao suportado nesta versao")` no parser ao formar o tipo |
| `float`/`double` (v1) | idem, ao formar o tipo ou ao ver literal de ponto flutuante |
| `_Thread_local`, `_Atomic`, `asm` | erro de compilação explícito |

## 3. Verificação do codegen durante a implementação

Para cada função reescrita, um teste em `test/kscc/` com um `.c` mínimo e a saída esperada do
programa (não o assembly esperado: o assembly pode mudar em otimizações). O `make test` compila,
monta com `ksas`, roda no `ksvm` e compara stdout/exit code (doc 06).

Ferramenta de inspeção: `kscc -S x.c -o -` imprime o assembly, e `ksvm --trace` mostra a
execução instrução a instrução. Os dois juntos substituem o `gdb` para depurar o codegen.
