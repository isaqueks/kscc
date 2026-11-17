// Um interpretador de bytecode de pilha (assembler mínimo + execução):
// programas para fatorial, soma de primos e GCD, com chamadas e retorno.
#include "kstest.h"

enum Op {
  OP_PUSH, OP_POP, OP_DUP, OP_SWAP, OP_OVER,
  OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
  OP_LT, OP_EQ, OP_NOT,
  OP_JMP, OP_JZ, OP_CALL, OP_RET,
  OP_LOAD, OP_STORE, OP_PRINT, OP_HALT
};

typedef struct {
  int stack[64];
  int sp;
  int rstack[16];
  int rsp;
  int mem[16];
  int pc;
  long steps;
  int out[16];
  int nout;
} VM;

static int run(VM *vm, const int *code) {
  vm->sp = vm->rsp = vm->pc = vm->nout = 0;
  vm->steps = 0;
  for (;;) {
    int op = code[vm->pc++];
    int a, b;
    vm->steps++;
    switch (op) {
    case OP_PUSH: vm->stack[vm->sp++] = code[vm->pc++]; break;
    case OP_POP: vm->sp--; break;
    case OP_DUP: vm->stack[vm->sp] = vm->stack[vm->sp - 1]; vm->sp++; break;
    case OP_SWAP:
      a = vm->stack[vm->sp - 1];
      vm->stack[vm->sp - 1] = vm->stack[vm->sp - 2];
      vm->stack[vm->sp - 2] = a;
      break;
    case OP_OVER: vm->stack[vm->sp] = vm->stack[vm->sp - 2]; vm->sp++; break;
#define BIN(OP, EXPR) \
  case OP: b = vm->stack[--vm->sp]; a = vm->stack[--vm->sp]; vm->stack[vm->sp++] = (EXPR); break;
    BIN(OP_ADD, a + b)
    BIN(OP_SUB, a - b)
    BIN(OP_MUL, a * b)
    BIN(OP_DIV, b ? a / b : 0)
    BIN(OP_MOD, b ? a % b : 0)
    BIN(OP_LT, a < b)
    BIN(OP_EQ, a == b)
#undef BIN
    case OP_NOT: vm->stack[vm->sp - 1] = !vm->stack[vm->sp - 1]; break;
    case OP_JMP: vm->pc = code[vm->pc]; break;
    case OP_JZ:
      a = vm->stack[--vm->sp];
      vm->pc = a ? vm->pc + 1 : code[vm->pc];
      break;
    case OP_CALL: vm->rstack[vm->rsp++] = vm->pc + 1; vm->pc = code[vm->pc]; break;
    case OP_RET: vm->pc = vm->rstack[--vm->rsp]; break;
    case OP_LOAD: vm->stack[vm->sp++] = vm->mem[code[vm->pc++]]; break;
    case OP_STORE: vm->mem[code[vm->pc++]] = vm->stack[--vm->sp]; break;
    case OP_PRINT: vm->out[vm->nout++] = vm->stack[--vm->sp]; break;
    case OP_HALT: return vm->sp ? vm->stack[vm->sp - 1] : 0;
    default: return -9999;
    }
  }
}

// fat(n) iterativo: mem0 = n, mem1 = acc
static const int prog_fact[] = {
    OP_PUSH, 10, OP_STORE, 0, OP_PUSH, 1, OP_STORE, 1,
    /* 8 */ OP_LOAD, 0, OP_JZ, 25,
    OP_LOAD, 1, OP_LOAD, 0, OP_MUL, OP_STORE, 1,
    OP_LOAD, 0, OP_PUSH, 1, OP_SUB, OP_STORE, 0,
    OP_JMP, 8,
    // o alvo do JZ (índice 11) é corrigido em main
};

// Programa montado em tempo de execução com rótulos resolvidos.
static int code[256];
static int ncode;
static int labels[16];
static int fixups[32][2];
static int nfix;

static void emit(int v) { code[ncode++] = v; }
static void emit_ref(int label) { fixups[nfix][0] = ncode; fixups[nfix][1] = label; nfix++; emit(0); }
static void mark(int label) { labels[label] = ncode; }
static void link_code(void) {
  for (int i = 0; i < nfix; i++) code[fixups[i][0]] = labels[fixups[i][1]];
  nfix = 0;
}

// is_prime(n) como sub-rotina: recebe n na pilha, devolve 0/1
static void build_primes(int limit) {
  ncode = 0;
  enum { L_LOOP = 1, L_END, L_ISP, L_ILOOP, L_NOTP, L_YES, L_SKIP };
  emit(OP_PUSH); emit(0); emit(OP_STORE); emit(1);    // soma
  emit(OP_PUSH); emit(2); emit(OP_STORE); emit(0);    // n
  mark(L_LOOP);
  emit(OP_LOAD); emit(0); emit(OP_PUSH); emit(limit); emit(OP_LT);
  emit(OP_JZ); emit_ref(L_END);
  emit(OP_LOAD); emit(0); emit(OP_CALL); emit_ref(L_ISP);
  emit(OP_JZ); emit_ref(L_SKIP);
  emit(OP_LOAD); emit(1); emit(OP_LOAD); emit(0); emit(OP_ADD); emit(OP_STORE); emit(1);
  emit(OP_LOAD); emit(0); emit(OP_PRINT);
  mark(L_SKIP);
  emit(OP_LOAD); emit(0); emit(OP_PUSH); emit(1); emit(OP_ADD); emit(OP_STORE); emit(0);
  emit(OP_JMP); emit_ref(L_LOOP);
  mark(L_END);
  emit(OP_LOAD); emit(1); emit(OP_HALT);
  // sub-rotina: mem2 = n, mem3 = d
  mark(L_ISP);
  emit(OP_STORE); emit(2); emit(OP_PUSH); emit(2); emit(OP_STORE); emit(3);
  mark(L_ILOOP);
  emit(OP_LOAD); emit(3); emit(OP_DUP); emit(OP_MUL); emit(OP_LOAD); emit(2); emit(OP_SWAP); emit(OP_LT);
  emit(OP_NOT); emit(OP_JZ); emit_ref(L_YES); // d*d <= n ?
  emit(OP_LOAD); emit(2); emit(OP_LOAD); emit(3); emit(OP_MOD); emit(OP_JZ); emit_ref(L_NOTP);
  emit(OP_LOAD); emit(3); emit(OP_PUSH); emit(1); emit(OP_ADD); emit(OP_STORE); emit(3);
  emit(OP_JMP); emit_ref(L_ILOOP);
  mark(L_NOTP);
  emit(OP_PUSH); emit(0); emit(OP_RET);
  mark(L_YES);
  emit(OP_PUSH); emit(1); emit(OP_RET);
  link_code();
}

static void build_gcd(int a, int b) {
  ncode = 0;
  enum { L_LOOP = 1, L_END };
  emit(OP_PUSH); emit(a); emit(OP_PUSH); emit(b);
  mark(L_LOOP);
  emit(OP_DUP); emit(OP_JZ); emit_ref(L_END);
  emit(OP_SWAP); emit(OP_OVER); emit(OP_MOD);
  emit(OP_JMP); emit_ref(L_LOOP);
  mark(L_END);
  emit(OP_POP); emit(OP_HALT);
  link_code();
}

static void report(const char *name, VM *vm, int r) {
  ks_put_str(name);
  ks_put_str(": result=");
  ks_put_int(r);
  ks_put_str(" steps=");
  ks_put_int((int)vm->steps);
  ks_put_str(" out=");
  for (int i = 0; i < vm->nout; i++) {
    ks_put_int(vm->out[i]);
    ks_put_chr(i + 1 < vm->nout ? ',' : ' ');
  }
  ks_put_chr('\n');
}

int main(void) {
  static VM vm;
  // conserta o alvo do JZ de prog_fact (fim = índice do HALT anexado)
  int fact[64];
  int n = (int)(sizeof prog_fact / sizeof prog_fact[0]);
  for (int i = 0; i < n; i++)
    fact[i] = prog_fact[i];
  fact[11] = n;
  fact[n] = OP_LOAD;
  fact[n + 1] = 1;
  fact[n + 2] = OP_HALT;
  report("fact", &vm, run(&vm, fact));
  build_primes(50);
  report("primes", &vm, run(&vm, code));
  build_gcd(1071, 462);
  report("gcd", &vm, run(&vm, code));
  build_gcd(832040, 514229);
  report("gcd fib", &vm, run(&vm, code));
  return 0;
}
