#!/bin/bash
#
# Download datasheets / EVT archives and extract them.
# データシート / EVT をダウンロードして解凍する。
# Run by GitHub Actions (.github/workflows/update.yml); the workflow does git commit/push.
# GitHub Actions から実行される。git の commit/push はワークフローが行う。
#
# On download failure, exit non-zero instead of overwriting a good file with an
# empty or HTML (404) response, so the Actions job fails.
# ダウンロード失敗時は空ファイルや HTML(404) で上書きせず非ゼロ終了し、ジョブを失敗させる。

set -euo pipefail

cd "$(dirname "$0")"

# Retry/timeout tuning (override via env, e.g. for tests).
: "${FETCH_RETRIES:=5}"
: "${FETCH_RETRY_DELAY:=10}"

# fetch <url> <output>
#   Download to a temp file, validate it, then atomically replace the target.
#   - HTTP 404/403/410 -> URL changed: fail immediately (no overwrite).
#   - empty / truncated / stalled / connection error -> transient: retry.
#   Validation: non-empty, PDF starts with %PDF, ZIP passes `unzip -t`.
#   If it never succeeds, return 1 and `set -e` fails the job, leaving the
#   existing good file untouched.
fetch() {
  local url="$1" out="$2"
  local tmp="${out}.download.$$"
  local attempt=0 code
  while [ "$attempt" -lt "$FETCH_RETRIES" ]; do
    attempt=$((attempt + 1))
    echo "Fetching ${out} <- ${url} (attempt ${attempt}/${FETCH_RETRIES})"
    code=$(curl -sSL \
                --connect-timeout 30 --max-time 900 \
                --speed-time 30 --speed-limit 1024 \
                -o "$tmp" -w '%{http_code}' "$url") || code="000"
    if [ "$code" = "200" ] && _valid "$tmp" "$out"; then
      mv -f "$tmp" "$out"
      echo "  saved ${out} ($(wc -c < "$out") bytes)"
      return 0
    fi
    rm -f "$tmp"
    case "$code" in
      404|403|410)
        echo "ERROR: ${url} returned HTTP ${code} (URL changed?)" >&2
        return 1
        ;;
    esac
    echo "  attempt ${attempt} failed (http=${code}); retrying in ${FETCH_RETRY_DELAY}s..." >&2
    sleep "$FETCH_RETRY_DELAY"
  done
  echo "ERROR: download failed for ${url} after ${FETCH_RETRIES} attempts" >&2
  return 1
}

# _valid <tmpfile> <outname>: true if the download looks complete and correct.
_valid() {
  local tmp="$1" out="$2"
  if [ ! -s "$tmp" ]; then
    echo "  -> empty response" >&2
    return 1
  fi
  case "${out,,}" in
    *.pdf)
      if [ "$(head -c 4 "$tmp")" != "%PDF" ]; then
        echo "  -> not a PDF" >&2
        return 1
      fi
      ;;
    *.zip)
      if ! unzip -tqq "$tmp" >/dev/null 2>&1; then
        echo "  -> not a valid/complete ZIP" >&2
        return 1
      fi
      ;;
  esac
  return 0
}

cd datasheet_en
# https://www.wch-ic.com/downloads/CH32V002DS0_PDF.html
fetch "https://www.wch-ic.com/download/file?id=397" CH32V002DS0.PDF
# https://www.wch-ic.com/downloads/CH32V004DS0_PDF.html
fetch "https://www.wch-ic.com/download/file?id=398" CH32V004DS0.PDF
# https://www.wch-ic.com/downloads/CH32V006DS0_PDF.html
fetch "https://www.wch-ic.com/download/file?id=396" CH32V006DS0.PDF
# https://www.wch-ic.com/downloads/CH32V00XRM_PDF.html
fetch "https://www.wch-ic.com/download/file?id=399" CH32V00XRM.PDF
# https://www.wch-ic.com/downloads/CH32V006DS0_PDF.html
fetch "https://www.wch-ic.com/download/file?id=405" CH32V007DS0.PDF
cd ..

cd datasheet_zh
# https://www.wch.cn/downloads/CH32V002DS0_PDF.html
fetch "https://file.wch.cn/download/file?id=473" CH32V002DS0.PDF
# https://www.wch.cn/downloads/CH32V004DS0_PDF.html
fetch "https://file.wch.cn/download/file?id=474" CH32V004DS0.PDF
# https://www.wch.cn/downloads/CH32V006DS0_PDF.html
fetch "https://file.wch.cn/download/file?id=475" CH32V006DS0.PDF
# https://www.wch.cn/downloads/CH32V00XRM_PDF.html
fetch "https://file.wch.cn/download/file?id=472" CH32V00XRM.PDF
# https://www.wch.cn/downloads/CH32V007DS0_PDF.html
fetch "https://file.wch.cn/download/file?id=476" CH32V007DS0.PDF
cd ..

# https://www.wch.cn/downloads/CH32V006EVT_ZIP.html
fetch "https://file.wch.cn/download/file?id=477" CH32V006EVT.ZIP
rm -rfv EVT
unzip -O GB2312 *.ZIP
