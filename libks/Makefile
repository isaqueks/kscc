# libks — biblioteca de runtime do kscc (doc 05 §5).
#
# Chamado pelo Makefile raiz:
#
#   make -C libks KSCC=/abs/build/kscc OUT=/abs/build/libks
#
# Cada src/*.c é compilado pelo próprio kscc (kscc -c, que valida a
# unidade com o ksas) e concatenado com os .ksasm de asm/ em
# $(OUT)/libks.kso, que o link usa com seleção por uso (src/link.c).
# crt0.ksasm vai para $(OUT) ao lado. Tudo depende do binário do
# compilador: a libks é recompilada sempre que o kscc muda, o que a torna
# um teste de regressão permanente do compilador.
#
#   KS_MALLOC_DEBUG=1   compila malloc.c com verificação de cabeçalhos e
#                       preenchimento 0xDD (doc 05 §3)

KSCC ?= ../build/kscc
OUT  ?= ../build/libks

KSCC_FLAGS ?=
ifeq ($(KS_MALLOC_DEBUG),1)
KSCC_FLAGS += -DKS_MALLOC_DEBUG
endif

SRCS = $(wildcard src/*.c)
ASMS = asm/syscall.ksasm asm/abort.ksasm
HDRS = $(wildcard include/*.h include/ksvm/*.h ../include/*.h)

C_OBJS   = $(SRCS:src/%.c=$(OUT)/obj/%.kso)
ASM_OBJS = $(ASMS:asm/%.ksasm=$(OUT)/obj/%.kso)

all: $(OUT)/libks.kso $(OUT)/crt0.ksasm

# Opções de compilação registradas num arquivo que só muda quando elas
# mudam, para que KS_MALLOC_DEBUG=1 recompile o que depende delas.
$(OUT)/flags: FORCE
	@mkdir -p $(@D)
	@echo '$(KSCC_FLAGS)' | cmp -s - $@ || echo '$(KSCC_FLAGS)' > $@

$(OUT)/obj/%.kso: src/%.c $(HDRS) $(KSCC) $(OUT)/flags Makefile
	@mkdir -p $(@D)
	$(KSCC) $(KSCC_FLAGS) -c -o $@ $<

# Os .ksasm também passam pelo kscc -c: o ksas os valida isoladamente.
$(OUT)/obj/%.kso: asm/%.ksasm $(KSCC) Makefile
	@mkdir -p $(@D)
	$(KSCC) -c -o $@ $<

# A concatenação é validada de novo inteira: um label global repetido em
# duas unidades da libks aparece aqui, e não no link do usuário.
$(OUT)/libks.kso: $(ASM_OBJS) $(C_OBJS)
	cat $^ > $@.tmp
	$(KSCC) -c -x assembler -o $@ $@.tmp
	rm -f $@.tmp

$(OUT)/crt0.ksasm: asm/crt0.ksasm $(KSCC) Makefile
	@mkdir -p $(@D)
	$(KSCC) -c -o $@ $<

clean:
	rm -rf $(OUT)

.PHONY: all clean FORCE
