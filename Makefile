# kscc — compilador C para a KSVM v2 (fork do chibicc).
#
#   make            constrói build/kscc e a libks (build/libks/)
#   make test       suite completa (unitários, chibicc adaptada, driver)
#   make test-asan  idem, com o compilador construído com ASan/UBSan
#   make clean
#
# Depende da ksvm2 construída (ksas, ksvm) em KSVM2_HOME.

KSVM2_HOME ?= ../ksvm2
KSAS       ?= $(KSVM2_HOME)/build/ksas
KSVM       ?= $(KSVM2_HOME)/build/tools/ksvm

CC      ?= cc
CFLAGS  ?= -std=c11 -g -fno-common -Wall -Wextra -Wno-switch -Werror
LDFLAGS ?=

ASAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined

SRCS = $(wildcard src/*.c)
HDRS = $(wildcard src/*.h)

# Caminho padrão do ksas embutido no binário (pode ser trocado em tempo de
# execução por --ksas, $KSAS ou $KSVM2_HOME).
DEFS = -DKSCC_DEFAULT_KSAS='"$(abspath $(KSAS))"'

OBJS      = $(SRCS:src/%.c=build/obj/%.o)
ASAN_OBJS = $(SRCS:src/%.c=build/asan/obj/%.o)

all: build/kscc libks

# A libks é compilada pelo próprio kscc e fica ao lado do binário que a
# compilou (build/libks, build/asan/libks), onde o driver a procura. O
# sub-make refaz tudo quando o compilador muda (libks/Makefile).
libks: build/kscc
	$(MAKE) -C libks KSCC=$(abspath build/kscc) OUT=$(abspath build/libks)

libks-asan: build/asan/kscc
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) -C libks KSCC=$(abspath build/asan/kscc) \
	  OUT=$(abspath build/asan/libks)

build/kscc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/obj/%.o: src/%.c $(HDRS) Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEFS) -c -o $@ $<

# Variante instrumentada (build/asan/kscc). O driver acha include/ e
# libks/include/ subindo a partir do diretório do binário (find_prefix em
# src/main.c), e a libks em <diretório do binário>/libks.
build/asan/kscc: $(ASAN_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $@ $^ $(LDFLAGS)

build/asan/obj/%.o: src/%.c $(HDRS) Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(ASAN_FLAGS) $(DEFS) -c -o $@ $<

check-ksvm2:
	@test -x $(KSAS) || { echo "ksas nao encontrado em $(KSAS) (KSVM2_HOME=$(KSVM2_HOME))"; exit 1; }
	@test -x $(KSVM) || { echo "ksvm nao encontrado em $(KSVM) (KSVM2_HOME=$(KSVM2_HOME))"; exit 1; }

test: build/kscc libks check-ksvm2
	KSAS=$(abspath $(KSAS)) KSVM=$(abspath $(KSVM)) test/run.sh build/kscc
	KSAS=$(abspath $(KSAS)) KSVM=$(abspath $(KSVM)) test/chibicc/driver.sh build/kscc

test-asan: build/asan/kscc libks-asan check-ksvm2
	ASAN_OPTIONS=detect_leaks=0 KSAS=$(abspath $(KSAS)) KSVM=$(abspath $(KSVM)) \
	  test/run.sh build/asan/kscc
	ASAN_OPTIONS=detect_leaks=0 KSAS=$(abspath $(KSAS)) KSVM=$(abspath $(KSVM)) \
	  test/chibicc/driver.sh build/asan/kscc

# Bateria completa: unit + chibicc + diferencial (gcc -m32) + fuzz + bench + embarcados
# (simavr/QEMU via ../ksvm2). Variáveis: FUZZ_N (padrão 200), CSMITH_N (padrão 30).
test-all: build/kscc libks check-ksvm2
	FUZZ_N=$(or $(FUZZ_N),200) CSMITH_N=$(or $(CSMITH_N),30) tools/run_all_tests.sh

clean:
	rm -rf build

.PHONY: all libks libks-asan test test-asan test-all clean check-ksvm2
