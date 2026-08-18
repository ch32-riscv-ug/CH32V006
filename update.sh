#!/bin/bash
#
# Download this mirror's documents and extract its EVT archive.
# このmirrorが担当する文書をダウンロードし、EVTを展開する。
# Run by GitHub Actions (.github/workflows/update.yml); the workflow does git commit/push.
# GitHub Actions から実行される。git の commit/push はワークフローが行う。
#
# What to fetch is not written here. It comes from the catalogue in
# ch32-riscv-ug/ch32-device-data, which records for every WCH document its download
# id per language and which mirror owns it. That assignment is a judgement -- one
# datasheet can cover several product families, the same document is published under
# two names, and a product with its own reference manual needs its own mirror -- so it
# is decided in one place rather than repeated in ten update scripts.
# 取得対象はここには書かない。ch32-device-data のカタログが、文書ごとの言語別
# download id と担当mirrorを持つ。どの文書がどのmirrorのものかは判断を要する
# ため、10箇所のスクリプトに複製せず1箇所で決める。
#
# Failure policy / 失敗時の方針:
#   - any HTTP error status (4xx incl. 404, and 5xx) -> fail the job (alert).
#     HTTP エラー応答(404 等の 4xx・5xx サーバエラー)はジョブを失敗させて通知する。
#   - transport-level glitches only (HTTP/2 PROTOCOL_ERROR, timeout, reset,
#     empty / truncated transfer) -> skip this file; the next daily run retries.
#     伝送レベルの一時障害はその回スキップし翌日再試行。
#   One request per file, no in-run retry — the daily schedule is the retry.
#   リトライせず1ファイル1回のみ。日次実行が再試行を兼ねる。

set -euo pipefail

cd "$(dirname "$0")"

# The mirror names itself; the catalogue says what belongs to it.
# mirror名は自分で名乗り、担当文書はカタログが決める。
REPOSITORY="${REPOSITORY:-$(basename "$(pwd)")}"
CATALOGUE_URL="${CATALOGUE_URL:-https://raw.githubusercontent.com/ch32-riscv-ug/ch32-device-data/main/manifests/documents.json}"
CATALOGUE_CACHE="documents.json"

HARD_FAILS=()     # HTTP error status (4xx/5xx) -> fail the job
SOFT_FAILS=()     # transport glitch -> skip this file, retry next run
LAST_FETCH_OK=0   # set by fetch(): 1 if the last call updated the file

# fetch <url> <output>
#   One attempt: download to a temp file, validate it, then atomically replace the
#   target. Forces HTTP/1.1 to avoid the intermittent HTTP/2 PROTOCOL_ERROR from the
#   CDN. Never aborts the script: outcomes go to HARD_FAILS / SOFT_FAILS.
fetch() {
  local url="$1" out="$2"
  local tmp="${out}.download.$$"
  local code
  LAST_FETCH_OK=0
  echo "Fetching ${out} <- ${url}"
  code=$(curl -sSL --http1.1 \
              --connect-timeout 30 --max-time 900 \
              --speed-time 30 --speed-limit 1024 \
              -o "$tmp" -w '%{http_code}' "$url") || code="000"
  if [ "$code" = "200" ] && _valid "$tmp" "$out"; then
    mv -f "$tmp" "$out"
    echo "  saved ${out} ($(wc -c < "$out") bytes)"
    LAST_FETCH_OK=1
    return 0
  fi
  rm -f "$tmp"
  if [ "$code" = "200" ] || [ "$code" = "000" ]; then
    echo "  -> transient failure (status=${code}); skipping, will retry next run" >&2
    SOFT_FAILS+=("${out}  ${url}")
  else
    echo "  -> HTTP ${code}: genuine error (URL changed or server error)" >&2
    HARD_FAILS+=("${out}  HTTP ${code}  ${url}")
  fi
}

# _valid <tmpfile> <outname>: true if the download looks complete and correct.
_valid() {
  local tmp="$1" out="$2"
  [ -s "$tmp" ] || { echo "  -> empty" >&2; return 1; }
  case "${out##*.}" in
    PDF|pdf)
      if [ "$(head -c 4 "$tmp")" != "%PDF" ]; then
        echo "  -> not a PDF" >&2
        return 1
      fi
      ;;
    ZIP|zip)
      if ! unzip -tqq "$tmp" >/dev/null 2>&1; then
        echo "  -> not a valid ZIP" >&2
        return 1
      fi
      ;;
  esac
  return 0
}

# unzip_evt <zipfile>: replace ./EVT with the contents of a freshly fetched zip.
unzip_evt() {
  rm -rf EVT
  unzip -O GB2312 "$1"
  rm -f "$1"
}

finish() {
  if [ ${#SOFT_FAILS[@]} -gt 0 ]; then
    echo "::warning::skipped ${#SOFT_FAILS[@]} download(s) due to transient errors; will retry next run"
    printf '  %s\n' "${SOFT_FAILS[@]}"
  fi
  if [ ${#HARD_FAILS[@]} -gt 0 ]; then
    echo "::error::${#HARD_FAILS[@]} download(s) failed with a genuine error (URL changed / server error)" >&2
    printf '  %s\n' "${HARD_FAILS[@]}" >&2
    exit 1
  fi
}

# Refresh the catalogue, but keep working from the committed copy if the fetch
# fails: a bad day upstream should not stop the mirror from updating its files.
# カタログ取得に失敗しても、commit済みの写しで動き続ける。
echo "Catalogue: ${CATALOGUE_URL}"
if curl -sSL --http1.1 --connect-timeout 30 --max-time 120 \
        -o "${CATALOGUE_CACHE}.new" "$CATALOGUE_URL" \
   && python3 -c "import json,sys; json.load(open(sys.argv[1]))" "${CATALOGUE_CACHE}.new"; then
  mv -f "${CATALOGUE_CACHE}.new" "$CATALOGUE_CACHE"
  echo "  updated ${CATALOGUE_CACHE}"
else
  rm -f "${CATALOGUE_CACHE}.new"
  if [ -f "$CATALOGUE_CACHE" ]; then
    echo "::warning::could not refresh the catalogue; using the committed copy"
  else
    echo "::error::no catalogue available (fetch failed and no committed copy)" >&2
    exit 1
  fi
fi

mkdir -p datasheet_en datasheet_zh

# Emit one "<language> <file_id> <name>" line per document assigned to this mirror.
# 担当文書を1行1件で書き出す。
plan() {
  REPOSITORY="$REPOSITORY" python3 - "$CATALOGUE_CACHE" <<'PYEOF'
import json, os, sys

catalogue = json.load(open(sys.argv[1], encoding="utf-8"))
mine = os.environ["REPOSITORY"]
for doc in catalogue["documents"]:
    if doc.get("status") != "assigned" or mine not in doc.get("repositories", []):
        continue
    for lang, src in sorted(doc.get("sources", {}).items()):
        if src.get("file_id") is not None:
            print(lang, src["file_id"], doc["name"])
PYEOF
}

PLAN="$(plan)"
if [ -z "$PLAN" ]; then
  echo "::error::the catalogue assigns no document to ${REPOSITORY}" >&2
  exit 1
fi
echo "${REPOSITORY}: $(printf '%s\n' "$PLAN" | wc -l) document(s) to fetch"

# An EVT archive is fetched at the top level and replaces ./EVT; PDFs go to the
# directory of their language. Only the Chinese site publishes some documents, and
# the two languages can be at different versions, so each is fetched on its own.
# EVTは直下で取得して ./EVT を置き換える。PDFは言語別ディレクトリへ。
while read -r lang id name; do
  [ -n "$name" ] || continue
  case "$lang" in
    en) url="https://www.wch-ic.com/download/file?id=${id}" ;;
    zh) url="https://file.wch.cn/download/file?id=${id}" ;;
    *)  echo "::error::unknown language '${lang}' in the catalogue" >&2; exit 1 ;;
  esac
  case "$name" in
    *.ZIP|*.zip)
      fetch "$url" "$name"
      if [ "$LAST_FETCH_OK" = 1 ]; then unzip_evt "$name"; fi
      ;;
    *)
      ( cd "datasheet_${lang}" && fetch "$url" "$name" )
      ;;
  esac
done <<< "$PLAN"

finish
