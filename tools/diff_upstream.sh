#!/bin/bash
# tools/diff_upstream.sh — diff de src/ (árvore de trabalho) contra o
# chibicc de origem (commit 90d1f7f), para revisão das mudanças do kscc
# (doc 06 §2, doc 07 §3).
#
#   tools/diff_upstream.sh [opções do git diff]
#
# Exemplos:
#   tools/diff_upstream.sh            diff completo
#   tools/diff_upstream.sh --stat     resumo por arquivo
#
# No chibicc os fontes ficavam na raiz do repositório e os headers do
# compilador em include/; no kscc, em src/ e include/. O script extrai os
# fontes do commit de origem para um diretório temporário e compara.
set -e
cd "$(dirname "$0")/.."
UPSTREAM=90d1f7f

tmp=$(mktemp -d /tmp/kscc-upstream-XXXXXX)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/chibicc"
git archive "$UPSTREAM" -- '*.c' '*.h' | tar -x -C "$tmp/chibicc"
rm -rf "$tmp/chibicc/test"

mkdir -p "$tmp/a" "$tmp/b"
cp "$tmp"/chibicc/*.c "$tmp"/chibicc/*.h "$tmp/a/"
cp src/*.c src/*.h "$tmp/b/"
mkdir -p "$tmp/a/include" "$tmp/b/include"
cp "$tmp"/chibicc/include/*.h "$tmp/a/include/"
cp include/*.h "$tmp/b/include/"

cd "$tmp"
git diff --no-index "$@" a b || true
