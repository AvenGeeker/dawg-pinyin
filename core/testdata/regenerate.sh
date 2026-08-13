#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Regenerates the checked-in testdata assets (tiny.dawg / tiny.clm).
# Usage: core/testdata/regenerate.sh [build_dir]
#   build_dir defaults to <repo>/build/core
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"        # .../core/testdata
CORE="$(dirname "$HERE")"                    # .../core
REPO="$(dirname "$CORE")"                    # repo root
BUILD="${1:-$REPO/build/core}"

"$BUILD/dawg_build_dict" "$HERE/dict/tiny_dict.txt" "$HERE/dict/tiny.dawg"

TMP_RAW="$(mktemp -t tiny_charlm.XXXXXX)"
python3 "$REPO/tools/lm/train_charlm.py" "$HERE/corpus/tiny_zh.txt" "$TMP_RAW" \
  --order 3 --vocab 128 --min-count 1 --top-k 32
python3 "$REPO/tools/lm/eval_lm.py" "$TMP_RAW" "$HERE/corpus/tiny_zh.txt"
python3 "$REPO/tools/lm/quantize_lm.py" "$TMP_RAW" "$HERE/lm/tiny.clm"
rm -f "$TMP_RAW"
echo "regenerated: $HERE/dict/tiny.dawg $HERE/lm/tiny.clm"
