#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/..")
EXECUTABLE="$PROJECT_ROOT/build/bin/zmq_publisher"
ENV_FILE="$SCRIPT_DIR/environment/.env.local"

CONFIG_PATH="$PROJECT_ROOT/config/config.json"
PRELOAD_LIBS=""
MIDAS_EXPTAB_PATH=""
MIDAS_EXPERIMENT=""
SETUP_PROFILE=""
DEBUG=false
VALGRIND=false
EXTRA_ARGS=()

show_help() {
    cat <<EOF
Usage: ./scripts/run.sh [OPTIONS] [-- PUBLISHER_ARGS...]

Options:
  --config PATH             Publisher configuration (default: config/config.json)
  --experiment NAME         MIDAS experiment name
  --exptab PATH             MIDAS experiment table
  --preload LIBS            Comma-separated libraries to preload
  --setup PROFILE           Create/validate the environment before launch
                            PROFILE: generic, midas, or gaudi
  -d, --debug               Run under gdb
  -v, --valgrind            Run under valgrind
  -h, --help                Display this help message

Machine-specific paths are loaded from scripts/environment/.env.local when it exists.
EOF
}

require_value() {
    if [[ $# -lt 2 || -z "${2:-}" || "${2:-}" == -* ]]; then
        echo "[run.sh, ERROR] $1 requires a value" >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --config) require_value "$@"; CONFIG_PATH=$2; shift 2 ;;
        --experiment) require_value "$@"; MIDAS_EXPERIMENT=$2; shift 2 ;;
        --exptab) require_value "$@"; MIDAS_EXPTAB_PATH=$2; shift 2 ;;
        --preload) require_value "$@"; PRELOAD_LIBS=${2//,/:}; shift 2 ;;
        --setup) require_value "$@"; SETUP_PROFILE=$2; shift 2 ;;
        -d|--debug) DEBUG=true; shift ;;
        -v|--valgrind) VALGRIND=true; shift ;;
        -h|--help) show_help; exit 0 ;;
        --) shift; EXTRA_ARGS+=("$@"); break ;;
        *) echo "[run.sh, ERROR] Unknown option: $1" >&2; show_help; exit 2 ;;
    esac
done

if [[ -n "$SETUP_PROFILE" ]]; then
    case "$SETUP_PROFILE" in
        generic) SETUP_ARGS=() ;;
        midas) SETUP_ARGS=(--with-midas) ;;
        gaudi) SETUP_ARGS=(--with-gaudi) ;;
        *)
            echo "[run.sh, ERROR] --setup must be generic, midas, or gaudi" >&2
            exit 2
            ;;
    esac
    "$SCRIPT_DIR/setup_env.sh" "${SETUP_ARGS[@]}"
fi

if [[ ! -x "$EXECUTABLE" ]]; then
    echo "[run.sh, ERROR] Executable not found: $EXECUTABLE" >&2
    echo "Build it with ./scripts/build.sh first." >&2
    exit 1
fi
if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

export ZMQ_PUBLISHER_ROOT="$PROJECT_ROOT"
if [[ -n "$MIDAS_EXPTAB_PATH" ]]; then
    export MIDAS_EXPTAB
    MIDAS_EXPTAB=$(realpath "$MIDAS_EXPTAB_PATH")
fi
if [[ -n "$MIDAS_EXPERIMENT" ]]; then
    export MIDAS_EXPT_NAME="$MIDAS_EXPERIMENT"
fi
if [[ -n "$PRELOAD_LIBS" ]]; then
    export LD_PRELOAD="$PRELOAD_LIBS${LD_PRELOAD:+:$LD_PRELOAD}"
fi

CONFIG_PATH=$(realpath "$CONFIG_PATH")
cd "$PROJECT_ROOT"
COMMAND=("$EXECUTABLE" --config "$CONFIG_PATH" "${EXTRA_ARGS[@]}")
if [[ "$DEBUG" == true ]]; then
    exec gdb --args "${COMMAND[@]}"
elif [[ "$VALGRIND" == true ]]; then
    exec valgrind --leak-check=full --track-origins=yes "${COMMAND[@]}"
else
    exec "${COMMAND[@]}"
fi
