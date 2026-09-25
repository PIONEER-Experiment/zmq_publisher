#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT=$(dirname "$(realpath "$0")")
ENVIRONMENT_DIR="$PROJECT_ROOT/scripts/environment"
WITH_MIDAS=false
DETECT_ARGS=()

show_help() {
    echo "Usage: ./setup_env.sh [OPTIONS] [SEARCH_ROOTS...]"
    echo
    echo "Checks the dependencies required by the generic publisher."
    echo "With --with-midas, also discovers MIDAS and writes scripts/environment/.env."
    echo
    echo "Options:"
    echo "  --with-midas              Discover the optional MIDAS environment"
    echo "  -d, --debug               Show MIDAS discovery details"
    echo "  -h, --help                Display this help message"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --with-midas) WITH_MIDAS=true; shift ;;
        -d|--debug) DETECT_ARGS+=("$1"); shift ;;
        -h|--help) show_help; exit 0 ;;
        --) shift; DETECT_ARGS+=("$@"); break ;;
        -*) echo "[setup_env.sh, ERROR] Unknown option: $1"; show_help; exit 1 ;;
        *) DETECT_ARGS+=("$1"); shift ;;
    esac
done

for command_name in cmake c++; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "[setup_env.sh, ERROR] Required command not found: $command_name"
        exit 1
    fi
done

if ! printf '#include <zmq.hpp>\n' | c++ -x c++ -E - >/dev/null 2>&1; then
    echo "[setup_env.sh, ERROR] cppzmq header zmq.hpp was not found by the compiler."
    exit 1
fi

if ! ldconfig -p 2>/dev/null | grep 'libzmq' >/dev/null; then
    echo "[setup_env.sh, WARN] libzmq was not found in the system linker cache."
    echo "[setup_env.sh, WARN] CMake will perform the authoritative dependency check."
fi

echo "[setup_env.sh] Generic publisher prerequisites are available."

if [ "$WITH_MIDAS" = true ]; then
    "$ENVIRONMENT_DIR/detect_environment.sh" "${DETECT_ARGS[@]}"
else
    echo "[setup_env.sh] MIDAS discovery skipped; it is not required for the default build."
fi
