#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Builds the production IME assets into assets/:
#   dawg_pinyin.dawg  lexicon  (Rime luna_pinyin -> import_rime -> build_dict)
#   dawg_pinyin.clm   char n-gram LM (zhwiki corpus -> train_charlm -> quantize)
#
# Requires network access (downloads Rime dictionary and zhwiki dump) and a
# host build of the core tools. Idempotent: reruns reuse downloads and
# intermediate files under build/lm-work/, which is never committed.
#
# Tunables (env): DAWG_LM_MAX_LINES (default 2000000 sentences).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="$ROOT/build/lm-work"
ASSETS="$ROOT/assets"
TOOLS="$ROOT/build/core"
PYTHON="${PYTHON:-python3}"
# rime-pinyin-simp: simplified lexicon with real integer word frequencies
# (Google Android PinyinIME lineage). luna_pinyin is traditional-only and
# carries only coarse percentage weights, so it is not the default.
LEXICON_URL="${DAWG_LEXICON_URL:-https://raw.githubusercontent.com/rime/rime-pinyin-simp/master/pinyin_simp.dict.yaml}"
# Corpus sources. The HF mirror of the filtered zhwiki extract is the
# default because dumps.wikimedia.org is heavily throttled from CN; set
# DAWG_CORPUS_SOURCE=zhwiki to use the official dump instead.
HF_CORPUS_URL="${DAWG_HF_CORPUS_URL:-https://hf-mirror.com/datasets/0xDing/wikipedia-cn-20230720-filtered/resolve/main/wikipedia-cn-20230720-filtered.json}"
WIKI_URL="${DAWG_WIKI_URL:-https://dumps.wikimedia.org/zhwiki/latest/zhwiki-latest-pages-articles.xml.bz2}"
CORPUS_SOURCE="${DAWG_CORPUS_SOURCE:-hf}"
MAX_LINES="${DAWG_LM_MAX_LINES:-2000000}"
# LCCC conversational corpus (everyday dialogue) mixed in after the wiki
# encyclopedia text.
LCCC_URL="${DAWG_LCCC_URL:-https://hf-mirror.com/datasets/silver/lccc/resolve/main/lccc_base_train.jsonl.gz}"
LCCC_LINES="${DAWG_LCCC_LINES:-1000000}"
DAWG_BUDGET=$((15 * 1024 * 1024))
CLM_BUDGET=$((40 * 1024 * 1024))

fail() { echo "ERROR: $*" >&2; exit 1; }

file_size() { wc -c < "$1" | tr -d ' '; }

fetch() { # <url> <dest>
  local url="$1" dest="$2"
  if [[ -s "$dest" ]]; then
    echo ">> reuse cached $(basename "$dest")"
    return
  fi
  echo ">> download $url"
  curl -fL --retry 3 -o "$dest.part" "$url" \
    || fail "download failed: $url (network access required; rerun with approval)"
  mv "$dest.part" "$dest"
}

[[ -x "$TOOLS/dawg_import_rime" && -x "$TOOLS/dawg_build_dict" && -x "$TOOLS/dawg_bench" ]] \
  || fail "host tools missing; run first: cmake -S core -B build/core && cmake --build build/core -j8"
mkdir -p "$WORK" "$ASSETS"

# ---------------------------------------------------------------- dictionary
echo "== [1/3] lexicon (rime-pinyin-simp -> .dawg) =="
LEXICON="$WORK/pinyin_simp.dict.yaml"
fetch "$LEXICON_URL" "$LEXICON"
DICT_TXT="$WORK/dict.txt"
DAWG_OUT="$ASSETS/dawg_pinyin.dawg"
"$TOOLS/dawg_import_rime" "$LEXICON" "$DICT_TXT"
"$TOOLS/dawg_build_dict" "$DICT_TXT" "$DAWG_OUT"
DAWG_SIZE=$(file_size "$DAWG_OUT")
echo ">> $DAWG_OUT ($(( DAWG_SIZE / 1024 )) KiB)"
(( DAWG_SIZE <= DAWG_BUDGET )) || fail "dawg exceeds 15MB budget: $DAWG_SIZE bytes"

# ------------------------------------------------------------------- corpus
echo "== [2/3] charlm (zhwiki + LCCC -> .clm) =="
WIKI_CORPUS="$WORK/wiki.norm.txt"
LCCC_CORPUS="$WORK/lccc.norm.txt"
CORPUS="$WORK/corpus.norm.txt"
if [[ ! -s "$WIKI_CORPUS" || $(wc -l < "$WIKI_CORPUS" | tr -d ' ') -lt "$MAX_LINES" ]]; then
  echo ">> streaming wiki corpus source=$CORPUS_SOURCE (early stop at $MAX_LINES sentences)"
  # The extractors exit as soon as enough sentences are collected, which
  # closes the pipe and aborts the download long before the full archive
  # transfers; hence `|| true` on both sides and the size gate below.
  if [[ "$CORPUS_SOURCE" == "zhwiki" ]]; then
    { curl -fL --retry 3 "$WIKI_URL" || true; } \
      | "$PYTHON" "$ROOT/tools/lm/prepare_corpus.py" - "$WIKI_CORPUS" \
          --wiki --max-lines "$MAX_LINES" || true
  else
    { curl -fL --retry 3 "$HF_CORPUS_URL" || true; } \
      | "$PYTHON" "$ROOT/tools/assets/fetch_wiki_json.py" - "$WIKI_CORPUS" \
          --max-lines "$MAX_LINES" || true
  fi
fi
if [[ ! -s "$LCCC_CORPUS" || $(wc -l < "$LCCC_CORPUS" | tr -d ' ') -lt "$LCCC_LINES" ]]; then
  echo ">> streaming LCCC conversational corpus (early stop at $LCCC_LINES sentences)"
  { curl -fL --retry 3 "$LCCC_URL" || true; } \
    | "$PYTHON" "$ROOT/tools/assets/fetch_lccc.py" - "$LCCC_CORPUS" \
        --max-lines "$LCCC_LINES" || true
fi
cat "$WIKI_CORPUS" "$LCCC_CORPUS" > "$CORPUS"
TOTAL=$(wc -l < "$CORPUS" | tr -d ' ')
(( TOTAL >= 10000 )) || fail "corpus too small ($TOTAL sentences); download likely truncated"
HELD=$(( TOTAL / 10 ))
TRAIN=$(( TOTAL - HELD ))
TRAIN_TXT="$WORK/train.norm.txt"
HELD_TXT="$WORK/heldout.norm.txt"
head -n "$TRAIN" "$CORPUS" > "$TRAIN_TXT"
tail -n "$HELD" "$CORPUS" > "$HELD_TXT"
echo ">> corpus: $TOTAL sentences (train=$TRAIN heldout=$HELD)"

# Tuning ladder when the quantized .clm busts the 40MB budget: raise
# --min-count, then lower --top-k, then lower --order.
# Defaults are calibrated (2026-08) for the wiki(2M)+LCCC(1M) mix on this
# corpus: order 6 quantizes to ~609MB and order 5 to ~400MB because every
# observed context keeps at least one child (trainer floor), so node count
# is bounded by context diversity, not by top-k. Order 3 with min-count 16
# lands 39.8MiB at heldout ppl ~127 (order 6 mix measured 88; gate 150).
CLM_OUT="$ASSETS/dawg_pinyin.clm"
RAW="$WORK/charlm.raw"
ORDER="${DAWG_LM_ORDER:-3}"; VOCAB=8190
MIN_COUNT="${DAWG_LM_MIN_COUNT:-16}"; TOP_K="${DAWG_LM_TOP_K:-32}"
while true; do
  echo ">> train order=$ORDER vocab=$VOCAB min-count=$MIN_COUNT top-k=$TOP_K"
  "$PYTHON" "$ROOT/tools/lm/train_charlm.py" "$TRAIN_TXT" "$RAW" \
    --order "$ORDER" --vocab "$VOCAB" --min-count "$MIN_COUNT" --top-k "$TOP_K"
  # Gate calibrated 2026-08 for the wiki(2M)+LCCC(1M) corpus mix on this
  # pipeline: wiki-only 2M measures ppl~106; 150 leaves headroom while still
  # catching a badly degraded model (>=1.5x). Fragmented CJK sentences
  # (non-CJK chars split sentences, matching runtime context semantics)
  # make the historical 30 unreachable at this corpus scale.
  "$PYTHON" "$ROOT/tools/lm/eval_lm.py" "$RAW" "$HELD_TXT" --max-ppl 150 \
    || fail "heldout perplexity gate failed (max-ppl 150)"
  "$PYTHON" "$ROOT/tools/lm/quantize_lm.py" "$RAW" "$CLM_OUT"
  CLM_SIZE=$(file_size "$CLM_OUT")
  echo ">> $CLM_OUT ($(( CLM_SIZE / 1024 / 1024 )) MiB)"
  if (( CLM_SIZE <= CLM_BUDGET )); then break; fi
  if   (( MIN_COUNT == 16 )); then MIN_COUNT=32
  elif (( TOP_K == 32 )); then TOP_K=16
  elif (( ORDER == 3 )); then ORDER=2
  else fail "clm still exceeds 40MB budget after tuning ladder"
  fi
  rm -f "$RAW"
done

# -------------------------------------------------------------------- smoke
echo "== [3/3] smoke load via dawg_bench =="
"$TOOLS/dawg_bench" "$DAWG_OUT" "$CLM_OUT" zhonghuarenmingongheguo
echo "OK: assets ready in $ASSETS"
