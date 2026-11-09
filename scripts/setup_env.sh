#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/..")
ENVIRONMENT_DIR="$SCRIPT_DIR/environment"
ENV_FILE="$ENVIRONMENT_DIR/.env.local"
ENV_TEMPLATE="$ENVIRONMENT_DIR/.env.example"
WITH_MIDAS=false
WITH_GAUDI=false
WITH_BENCHMARKS=false

show_help() {
    echo "Usage: ./scripts/setup_env.sh [OPTIONS]"
    echo
    echo "Creates scripts/environment/.env.local from the committed template when absent,"
    echo "then validates the explicitly configured development environment."
    echo
    echo "Options:"
    echo "  --with-midas              Validate the optional MIDAS toolchain"
    echo "  --with-gaudi              Validate MIDAS and PIONEER Gaudi paths"
    echo "  --with-benchmarks         Create .venv and install rate-report dependencies"
    echo "  -h, --help                Display this help message"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --with-midas) WITH_MIDAS=true; shift ;;
        --with-gaudi) WITH_MIDAS=true; WITH_GAUDI=true; shift ;;
        --with-benchmarks) WITH_BENCHMARKS=true; shift ;;
        -h|--help) show_help; exit 0 ;;
        -*) echo "[setup_env.sh, ERROR] Unknown option: $1"; show_help; exit 1 ;;
        *) echo "[setup_env.sh, ERROR] Unexpected argument: $1"; show_help; exit 1 ;;
    esac
done

if [[ ! -f "$ENV_FILE" ]]; then
    cp "$ENV_TEMPLATE" "$ENV_FILE"
    echo "[setup_env.sh] Created $ENV_FILE"
    echo "[setup_env.sh] Edit that file for this machine, then rerun this command."
fi

# This file is deliberately shell syntax so it can also be sourced directly by
# developers and build scripts.
# shellcheck disable=SC1090
source "$ENV_FILE"

require_file() {
    local path=$1
    local description=$2
    if [[ ! -f "$path" ]]; then
        echo "[setup_env.sh, ERROR] $description was not found: $path"
        echo "[setup_env.sh, ERROR] Edit $ENV_FILE; automatic filesystem discovery is not used."
        exit 1
    fi
}

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
    if [[ -z "${MIDASSYS:-}" ]]; then
        echo "[setup_env.sh, ERROR] MIDASSYS is not configured in $ENV_FILE"
        exit 1
    fi
    require_file "$MIDASSYS/include/midas.h" "MIDAS header"
    require_file "$MIDASSYS/lib/libmidas.a" "MIDAS library"

    MIDAS_RECEIVER_SOURCE=${MIDAS_RECEIVER_SOURCE:-$PROJECT_ROOT/../midas_receiver}
    require_file "$MIDAS_RECEIVER_SOURCE/CMakeLists.txt" "midas_receiver source tree"
    echo "[setup_env.sh] MIDAS: $MIDASSYS"
    echo "[setup_env.sh] midas_receiver: $MIDAS_RECEIVER_SOURCE"
else
    echo "[setup_env.sh] MIDAS validation skipped; it is not required for the default build."
fi

if [ "$WITH_GAUDI" = true ]; then
    if [[ -z "${PIONEER_SOURCE_DIR:-}" ]]; then
        echo "[setup_env.sh, ERROR] PIONEER_SOURCE_DIR is not configured in $ENV_FILE"
        exit 1
    fi
    require_file \
        "$PIONEER_SOURCE_DIR/reco_testbeam/online_dqm/CMakeLists.txt" \
        "PIONEER online_dqm package"
    require_file \
        "$PROJECT_ROOT/extensions/gaudi/scripts/build_sidecar.sh" \
        "publisher Gaudi sidecar build script"
    if [[ -z "${PIONEER_GAUDI_IMAGE:-}" ]]; then
        echo "[setup_env.sh, ERROR] PIONEER_GAUDI_IMAGE is not configured in $ENV_FILE"
        exit 1
    fi
    if [[ ! -f "$PIONEER_GAUDI_IMAGE" ]]; then
        echo "[setup_env.sh, WARN] Gaudi image does not exist yet: $PIONEER_GAUDI_IMAGE"
        echo "[setup_env.sh, WARN] Create it before building the sidecar; the publisher build can proceed."
    else
        echo "[setup_env.sh] Gaudi image: $PIONEER_GAUDI_IMAGE"
    fi
    echo "[setup_env.sh] PIONEER source: $PIONEER_SOURCE_DIR"
fi

if [ "$WITH_BENCHMARKS" = true ]; then
    if ! command -v python3 >/dev/null 2>&1; then
        echo "[setup_env.sh, ERROR] python3 is required for the benchmark environment."
        exit 1
    fi
    if [[ ! -x "$PROJECT_ROOT/.venv/bin/python" ]]; then
        echo "[setup_env.sh] Creating Python environment: $PROJECT_ROOT/.venv"
        python3 -m venv "$PROJECT_ROOT/.venv"
    fi
    echo "[setup_env.sh] Synchronizing benchmark Python dependencies"
    "$PROJECT_ROOT/.venv/bin/python" -m pip install --quiet --disable-pip-version-check \
        --requirement "$PROJECT_ROOT/benchmarks/rate_tests/requirements.txt"
    echo "[setup_env.sh] Benchmark Python: $PROJECT_ROOT/.venv/bin/python"
fi

echo "[setup_env.sh] Environment validation complete."
