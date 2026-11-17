# kscc — compilador C para a KSVM v2

Fork do [chibicc](https://github.com/rui314/chibicc) (Rui Ueyama, MIT) com o gerador de
código retargetado para a máquina virtual [KSVM v2](https://github.com/isaqueks/ksvm2).

Estado: fases 0 e 1 (driver, link por concatenação, inteiros, controle de fluxo e
chamadas). Plano completo em `docs/README.md`.

Depende da ksvm2 construída (`ksas`, `ksvm`) em `KSVM2_HOME` (padrão `../ksvm2`).

    make                # build/kscc
    make test           # unitários (test/unit), suite do chibicc adaptada, driver.sh
    make test-asan      # idem com o compilador sob ASan/UBSan
    build/kscc hello.c -o hello.ksx && ../ksvm2/build/tools/ksvm hello.ksx
