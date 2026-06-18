#!/bin/bash
#
# Download datasheets / EVT archives and extract them.
# データシート / EVT をダウンロードして解凍する。
# Run by GitHub Actions (.github/workflows/update.yml); the workflow does git commit/push.
# GitHub Actions から実行される。git の commit/push はワークフローが行う。
#
# Failure policy / 失敗時の方針:
#   - any HTTP error status (4xx incl. 404, and 5xx) -> fail the job (alert).
#     HTTP エラー応答(404 等の 4xx・5xx サーバエラー)はジョブを失敗させて通知する。
#   - transport-level glitches only (HTTP/2 PROTOCOL_ERROR, timeout, reset,
#     empty / truncated transfer) -> retry, then skip; next daily run retries.
#     伝送レベルの一時障害(CDN のストリーム切断/タイムアウト等)だけスキップし翌日再試行。
#   Updates land only every few months, so skipping a day is harmless.
#   更新は数ヶ月単位なので1日スキップしても問題ない。

set -euo pipefail

cd "$(dirname "$0")"

# Retry/timeout tuning (override via env, e.g. for tests).
: "${FETCH_RETRIES:=5}"
: "${FETCH_RETRY_DELAY:=10}"

HARD_FAILS=()     # HTTP error status (4xx/5xx) -> fail the job
SOFT_FAILS=()     # transport-level glitches -> skip, retry next run
LAST_FETCH_OK=0   # set by fetch(): 1 if the last call updated the file

# fetch <url> <output>
#   Download to a temp file, validate it, then atomically replace the target.
#   Forces HTTP/1.1 to avoid the intermittent HTTP/2 PROTOCOL_ERROR from the CDN.
#   Never aborts the script: outcomes go to HARD_FAILS / SOFT_FAILS, judged by
#   finish() at the end. Sets LAST_FETCH_OK so the caller can decide whether to
#   re-extract a ZIP.
#
#   curl errors (transport-level) collapse to code "000" via the `|| code=...`.
#     200 + valid                 -> success
#     200 + empty/truncated       -> transient (retry, then soft skip)
#     "000" (curl transport error)-> transient (retry, then soft skip)
#     any other HTTP status (4xx/5xx) -> genuine error (hard fail, no retry)
fetch() {
  local url="$1" out="$2"
  local tmp="${out}.download.$$"
  local attempt=0 code
  LAST_FETCH_OK=0
  while [ "$attempt" -lt "$FETCH_RETRIES" ]; do
    attempt=$((attempt + 1))
    echo "Fetching ${out} <- ${url} (attempt ${attempt}/${FETCH_RETRIES})"
    code=$(curl -sSL --http1.1 \
                --connect-timeout 30 --max-time 900 \
                --speed-time 30 --speed-limit 1024 \
                -o "$tmp" -w '%{http_code}' "$url") || code="000"
    if [ "$code" = "200" ]; then
      if _valid "$tmp" "$out"; then
        mv -f "$tmp" "$out"
        echo "  saved ${out} ($(wc -c < "$out") bytes)"
        LAST_FETCH_OK=1
        return 0
      fi
      rm -f "$tmp"
      echo "  attempt ${attempt}: HTTP 200 but incomplete/invalid (transient); retrying in ${FETCH_RETRY_DELAY}s..." >&2
    elif [ "$code" = "000" ]; then
      rm -f "$tmp"
      echo "  attempt ${attempt}: transport error (curl, e.g. HTTP/2/timeout/reset); retrying in ${FETCH_RETRY_DELAY}s..." >&2
    else
      rm -f "$tmp"
      echo "  -> HTTP ${code}: genuine error (URL changed or server error)" >&2
      HARD_FAILS+=("${out}  HTTP ${code}  ${url}")
      return 0
    fi
    sleep "$FETCH_RETRY_DELAY"
  done
  echo "  -> giving up for now after ${FETCH_RETRIES} transient failures" >&2
  SOFT_FAILS+=("${out}  ${url}")
  return 0
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

# unzip_evt <zipfile>: replace ./EVT with the contents of a freshly fetched zip.
unzip_evt() {
  rm -rfv EVT
  unzip -O GB2312 "$1"
}

# finish: report results and set the job exit status.
#   - any genuine HTTP error (4xx/5xx) -> exit 1 (fail the job, alert).
#   - transient-only                   -> exit 0 (retry next run).
finish() {
  if [ "${#SOFT_FAILS[@]}" -gt 0 ]; then
    echo "::warning::skipped ${#SOFT_FAILS[@]} download(s) due to transient errors; will retry next run"
    printf '  - %s\n' "${SOFT_FAILS[@]}"
  fi
  if [ "${#HARD_FAILS[@]}" -gt 0 ]; then
    echo "::error::${#HARD_FAILS[@]} download(s) failed with a genuine error (URL changed / server error)" >&2
    printf '  - %s\n' "${HARD_FAILS[@]}" >&2
    exit 1
  fi
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
if [ "$LAST_FETCH_OK" = 1 ]; then unzip_evt CH32V006EVT.ZIP; fi

finish
