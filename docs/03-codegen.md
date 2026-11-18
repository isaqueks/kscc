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
| 4 bytes (inclusive `float`/`double`/`long double`) | `ldw r1, [r1]` |

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
Ponto flutuante: `fcmp r1, zr` (os bits de `zr` são +0.0). Z = 1 só para ±0, então `beq`/`bne`
dão o `== 0` do C: **−0 é falso, NaN (unordered, Z = 0) é verdadeiro**. Todos os usuários de
`cmp_zero` (`if`, laços, `?:`, `!`, `&&`, `||`, cast para `_Bool`) desviam por Z.
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
`setb`/`setbe` vs `setl`/`setle`). Ponto flutuante: `fcmp r1, r2` e as condições do ARM VFP (doc
04 §5.7 da ksvm2), corretas com NaN:

| C | depois de `fcmp a, b` | com NaN |
|---|-----------------------|---------|
| `a == b` | `beq` | falso |
| `a != b` | `bne` | **verdadeiro** |
| `a < b` | `bmi` | falso |
| `a <= b` | `bls` | falso |
| `a > b`, `a >= b` | o parser troca os lados (`b < a`, `b <= a`): `bmi`/`bls` | falso |

`!(a < b)` é `ND_NOT` sobre o 0/1 da comparação, então vale 1 com NaN (≠ `a >= b`, que vale 0).
Nunca se usa `cmp` inteiro para comparar floats (`+0 == -0` e NaN dariam errado).

Otimização óbvia para a fase 7: quando a comparação é usada
diretamente em `if`/`while`/`for`/`?:`/`&&`/`||`, fundir no desvio e não materializar 0/1.

### `cast(from, to)`
A tabela `cast_table[][]` do chibicc (10×10 de strings) vira uma tabela 7×7 sobre as classes
`i8, i16, i32, u8, u16, u32, f32` (`f32` = `float`, `double` e `long double`; `long long` entra
na fase 5), mais `bool` tratado à parte.

| origem → destino | KSVM |
|------------------|------|
| inteiro → `i8` / `u8` / `i16` / `u16` | `sxb` / `zxb` / `sxh` / `zxh r1, r1` |
| inteiro → `i32`/`u32`/ponteiro | nada |
| `i8 i16 i32 u8 u16` → `f32` | `itof r1, r1` (com sinal; RNE) |
| `u32` → `f32` | `utof r1, r1` (RNE: `0xFFFFFFFF` → 2³²) |
| `f32` → `i32` | `ftoi r1, r1` (trunca; satura em `INT32_MIN/MAX`; NaN → 0) |
| `f32` → `u32` | `ftou r1, r1` (trunca; negativos → 0, ≥ 2³² → `UINT32_MAX`; NaN → 0) |
| `f32` → `i8` / `u8` / `i16` / `u16` | `ftoi r1, r1` e depois `sxb` / `zxb` / `sxh` / `zxh` (como o `cvttss2si` do gcc) |
| `f32` → `f32` | nada (os três tipos são binary32) |
| qualquer → `bool` | `cmp_zero` (`cmp` ou `fcmp` com `zr`) + `movi r1, 1; bne L; movi r1, 0; L:` |

Fora do alcance a conversão float → inteiro é UB em C; o kscc dá o resultado definido da VM, o
mesmo em todo host (e o mesmo nas constantes dobradas pelo parser, §2).

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

### Ponto flutuante (fase 6)
Valores `float`/`double`/`long double` vivem em `r1` como os bits do binary32; load/store/push/
pop/argumentos/retorno são os de um `int` de 32 bits. As operações usam a extensão F da VM,
**inline, sem rotinas de runtime**, e nunca `add`/`cmp` inteiros:

| C | KSVM |
|---|------|
| `+` `-` `*` `/` | `fadd` / `fsub` / `fmul` / `fdiv r1, r1, r2` (x/±0 = ±inf, 0/0 = NaN, sem fault) |
| unário `-` | `fneg r1, r1` (só o sinal: `-0.0f` a partir de `0.0f`, também em NaN) |
| `==` `!=` `<` `<=` (e `>` `>=` trocados) | `fcmp r1, r2` + `set_cond` com `eq`/`ne`/`mi`/`ls` (tabela acima) |
| valor de verdade | `fcmp r1, zr` (`cmp_zero`) |
| casts | `itof`/`utof`/`ftoi`/`ftou` (tabela de `cast`) |
| literal | `li r1, <bits> ; valor` (os bits calculados no host; mesmo efeito de `fli`) |
| `x++`, `x--` | `p = &x, old = *p, *p = old + 1, old` (o `(x += 1) - 1` do chibicc arredonda) |
| `%`, `& \| ^ << >> ~` com float | erro de compilação (`add_type`) |
| `switch` com expressão float, `case 1.5`, `int a[2.5]`, `#if 1.5` | erro de compilação |

Builtins (parse.c, `float_builtin`; nó `ND_FBUILTIN`), uma instrução cada:

| builtin (também sem sufixo e com `l`) | KSVM |
|---------------------------------------|------|
| `__builtin_sqrtf` `fabsf` `floorf` `ceilf` `truncf` `rintf` `nearbyintf` | `fsqrt` `fabs` `ffloor` `fceil` `ftrunc` `frint` `frint r1, r1` |
| `__builtin_fminf(x, y)` `fmaxf` | `fmin` / `fmax r1, r1, r2` (um NaN: o outro operando) |
| `__builtin_isnan(x)` | `fcmp r1, r1` + `bvs` |
| `__builtin_isinf(x)` / `isfinite(x)` | `fabs r1, r1; li r2, 0x7F800000; fcmp r1, r2` + `beq` / `bmi` |
| `__builtin_signbit(x)` | `shri r1, r1, 31` |
| `__builtin_inff()` `huge_valf()` `nanf("")` | constantes (+inf, `0x7FC00000`; o payload do `nan` é ignorado) |

`<math.h>` da libks mapeia `sqrtf`, `fabs`, `floor`... para esses builtins (doc 05). Ao contrário do gcc, o kscc não dobra esses builtins em expressões constantes: `static float r = __builtin_sqrtf(2);` é erro ("not a compile-time constant"); as constantes `inf`/`nan` são constantes de verdade (`INFINITY` e `NAN` servem em inicializadores de globais).

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
Detalhe: como no chibicc atual, o endereço da função é avaliado **depois** dos argumentos e
movido para `r5` (o papel do `%r10` no x86-64); o passo 1 acima descreve a ordem lógica, não a
ordem de emissão.

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
| Constantes de ponto flutuante (inicializadores de globais, `static`, expressões constantes) | `eval_double` avalia em **binary32**: cada `+ - * /` é feito em `double` no host e arredondado para `float` (para dois operandos binary32 o arredondamento duplo é inócuo: 53 ≥ 2·24 + 2), e um NaN vira o canônico `0x7FC00000` (o x86 dá `0xFFC00000`). Float → inteiro com a saturação da VM (`float_to_int`); comparações, `!`, `&&`, `\|\|` e `?:` com operandos float usam o valor de ponto flutuante. Resultado: a constante dobrada tem o mesmo padrão de bits que a mesma conta em tempo de execução (`test/unit/float_const_fold.c`, `test/diff/float_const_fold.c`), como no `gcc -fsingle-precision-constant` |
| `_Thread_local`, `_Atomic`, `asm` | erro de compilação explícito |

## 3. Verificação do codegen durante a implementação

Para cada função reescrita, um teste em `test/kscc/` com um `.c` mínimo e a saída esperada do
programa (não o assembly esperado: o assembly pode mudar em otimizações). O `make test` compila,
monta com `ksas`, roda no `ksvm` e compara stdout/exit code (doc 06).

Ferramenta de inspeção: `kscc -S x.c -o -` imprime o assembly, e `ksvm --trace` mostra a
execução instrução a instrução. Os dois juntos substituem o `gdb` para depurar o codegen.
