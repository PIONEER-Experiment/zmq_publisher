#!/bin/bash
# detect_environment.sh - auto-detect MIDASSYS, MIDAS_EXPTAB, MIDAS_EXPT_NAME, ZeroMQ, cppzmq and write .env file

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

source "$SCRIPT_DIR/.helpers.sh"

DEBUG=false

show_help() {
  cat << EOF
Usage: $0 [OPTIONS] [SEARCH_ROOTS...]

Options:
  -d, --debug       Enable debug output
  -h, --help        Show this help message and exit

Positional arguments:
  SEARCH_ROOTS      One or more directories to search for dependencies.
                    Defaults to: \$HOME /opt /usr/local /usr
EOF
}

# Default search roots
SEARCH_ROOTS=("$HOME" "/opt" "/usr/local")

# Parse CLI args
while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--debug) DEBUG=true; shift ;;
    -h|--help) show_help; exit 0 ;;
    --) shift; break ;;
    -*) echo "Unknown option: $1" >&2; show_help; exit 1 ;;
    *) break ;;
  esac
done

# If positional args provided, override SEARCH_ROOTS
if [[ $# -gt 0 ]]; then
  SEARCH_ROOTS=()
  while [[ $# -gt 0 ]]; do
    SEARCH_ROOTS+=("$1")
    shift
  done
fi

# Required file patterns
REQUIRED_MIDAS_FILES=("MidasConfig.cmake" "include/midas.h")
REQUIRED_ZMQ_FILES=("include/zmq.h")
REQUIRED_CPPZMQ_FILES=("zmq.hpp")

ENV_FILE="$SCRIPT_DIR/.env"

echo "[INFO] Searching for dependencies..."
$DEBUG && echo "[DEBUG] Searching in roots: ${SEARCH_ROOTS[*]}" >&2

# --- MIDAS ---
echo "[INFO] Searching for MIDAS installation directories containing files: ${REQUIRED_MIDAS_FILES[*]}"
if found=$(find_root_with_files "${SEARCH_ROOTS[@]}" -- "${REQUIRED_MIDAS_FILES[@]}" "$DEBUG"); then
  echo "[INFO] Found MIDASSYS: $found"
  MIDASSYS="$found"
else
  echo "[WARN] MIDAS installation not found."
  MIDASSYS=""
fi

# --- MIDAS_EXPTAB ---
MIDAS_EXPTAB=""
MIDAS_EXPT_NAME=""

if [[ -n "$MIDASSYS" ]]; then
  echo "[INFO] Searching for MIDAS experiment table files named 'exptab' under roots: ${SEARCH_ROOTS[*]}"
  if exptab_path=$(find_file_in_candidates "${SEARCH_ROOTS[@]}" -- "exptab" "$DEBUG"); then
    echo "[INFO] Found MIDAS_EXPTAB: $exptab_path"
    MIDAS_EXPTAB="$exptab_path"

    # Extract experiment name (first word of first line)
    MIDAS_EXPT_NAME=$(head -n1 "$exptab_path" | awk '{print $1}')
    if [[ -n "$MIDAS_EXPT_NAME" ]]; then
      echo "[INFO] MIDAS_EXPT_NAME set to: $MIDAS_EXPT_NAME"
    else
      echo "[WARN] Could not extract MIDAS_EXPT_NAME from $exptab_path"
      MIDAS_EXPT_NAME=""
    fi
  else
    echo "[WARN] MIDAS_EXPTAB file not found."
  fi
fi

# --- ZeroMQ: find all matches ---
echo "[INFO] Searching for ZeroMQ installations (files: ${REQUIRED_ZMQ_FILES[*]}) in roots: ${SEARCH_ROOTS[*]}"
if found_list=$(find_root_with_files --all "${SEARCH_ROOTS[@]}" -- "${REQUIRED_ZMQ_FILES[@]}" "$DEBUG"); then
  echo "[INFO] Found ZeroMQ candidates:"
  echo "$found_list" | sed 's/^/  - /'
  echo "[INFO] If ZeroMQ is not installed system-wide, you can manually set ZEROMQ_ROOT in .env to one of the above."
else
  echo "[WARN] ZeroMQ not found."
fi

# --- cppzmq: find all matches ---
echo "[INFO] Searching for cppzmq installations (files: ${REQUIRED_CPPZMQ_FILES[*]}) in roots: ${SEARCH_ROOTS[*]}"
if found_list=$(find_root_with_files --all "${SEARCH_ROOTS[@]}" -- "${REQUIRED_CPPZMQ_FILES[@]}" "$DEBUG"); then
  echo "[INFO] Found cppzmq candidates:"
  echo "$found_list" | xargs -I{} dirname {} | sort -u | sed 's/^/  - /'
  echo "[INFO] If cppzmq is not installed system-wide, you can manually set CPPZMQ_ROOT in .env to one of the above."
else
  echo "[WARN] cppzmq not found."
fi

# --- Write .env file ---
echo "[INFO] Writing environment file: $ENV_FILE"
{
  [[ -n "$MIDASSYS" ]] && echo "export MIDASSYS=$MIDASSYS"
  [[ -n "$MIDAS_EXPTAB" ]] && echo "export MIDAS_EXPTAB=$MIDAS_EXPTAB"
  [[ -n "$MIDAS_EXPT_NAME" ]] && echo "export MIDAS_EXPT_NAME=$MIDAS_EXPT_NAME"
  # Add the unpacker path environment variable unconditionally:
  echo "export UNPACKER_PATH=$PROJECT_ROOT/build/_deps/unpacker-src"
  # Don't write ZEROMQ_ROOT or CPPZMQ_ROOT automatically
} > "$ENV_FILE"

echo "[INFO] Done. Run 'source $ENV_FILE' before building."
echo "[INFO] Note: ZeroMQ and cppzmq roots are not set automatically."
echo "[INFO] If needed, edit '$ENV_FILE' to add ZEROMQ_ROOT or CPPZMQ_ROOT manually from the listed candidates."
