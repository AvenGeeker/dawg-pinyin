#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Syncs IME assets into the entry rawfile dir: production assets from the
# repo assets/ when present, otherwise the tiny testdata fallbacks (any
# fresh clone can build).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$HERE/../.."
DEST="$HERE/entry/src/main/resources/rawfile"
mkdir -p "$DEST"
sync() { # <name> <prod> <fallback>
  if [[ -f "$2" ]]; then cp -f "$2" "$DEST/$1"; else cp -f "$3" "$DEST/$1"; fi
  echo "synced $1"
}
sync dawg_pinyin.dawg "$ROOT/assets/dawg_pinyin.dawg" "$ROOT/core/testdata/dict/tiny.dawg"
sync dawg_pinyin.clm "$ROOT/assets/dawg_pinyin.clm" "$ROOT/core/testdata/lm/tiny.clm"
