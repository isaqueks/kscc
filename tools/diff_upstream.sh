#!/bin/bash
# tools/diff_upstream.sh — diff de src/ contra o chibicc de origem
# (commit 90d1f7f), para revisão das mudanças do kscc (doc 06 §2).
#
#   tools/diff_upstream.sh [opções do git diff] [-- arquivos]
#
# Exemplos:
#   tools/diff_upstream.sh                 diff completo de src/
#   tools/diff_upstream.sh --stat          resumo por arquivo
#   tools/diff_upstream.sh -- src/type.c   só um arquivo
#
# Os testes adaptados ficam em test/chibicc/; para eles use
#   git diff 90d1f7f:test <commit>:test/chibicc  (layout mudou na reorganização).
set -e
cd "$(dirname "$0")/.."
UPSTREAM=90d1f7f

if [ "$#" -gt 0 ] && [[ " $* " == *" -- "* ]]; then
  exec git diff "$UPSTREAM" "$@"
fi
# No chibicc os fontes ficavam na raiz; o kscc os moveu para src/. O
# rename detection (-M) do git casa os arquivos.
exec git diff -M "$UPSTREAM" "$@" -- src/
