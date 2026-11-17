# Build e testes (ambiente de desenvolvimento)

Toolchain no WSL (Ubuntu 24.04: gcc 13, clang 18, make, gcc-multilib para `gcc -m32`).
Não há compilador C no Windows.

Caminho no WSL: `/mnt/c/Users/isaque.kanitz/KSVM2/kscc`
ksvm2 já construída (ksas, ksvm, ksdis, ksimg) em `/mnt/c/Users/isaque.kanitz/KSVM2/ksvm2/build`
(`build/ksas`, `build/tools/ksvm`, `build/tools/ksdis`, `build/tools/ksimg`). Se faltar:
`cd ../ksvm2 && make ksas tools`.

    wsl.exe -e bash -lc 'cd /mnt/c/Users/isaque.kanitz/KSVM2/kscc && make test'

Regras:
- Arquivos LF, UTF-8. Nunca editar `docs/` sem motivo: os docs são a especificação.
- O kscc é código de host: `-std=c11 -Wall -Wextra -Wno-switch -Werror` (herdado do chibicc, + -Wextra).
- `make test` deve ficar verde antes de declarar qualquer fase pronta; `make test-asan` idem.
