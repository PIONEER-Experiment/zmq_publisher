#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PUBLISHER_ROOT=$(realpath "$SCRIPT_DIR/../../..")
ENV_FILE=$PUBLISHER_ROOT/scripts/environment/.env.local
if [[ ! -f "$ENV_FILE" ]]; then
    echo "[run_sidecar, ERROR] Run ./setup_env.sh --with-gaudi first." >&2
    exit 1
fi
# shellcheck disable=SC1090
source "$ENV_FILE"
: "${PIONEER_SOURCE_DIR:?Set PIONEER_SOURCE_DIR in $ENV_FILE}"
: "${PIONEER_GAUDI_IMAGE:?Set PIONEER_GAUDI_IMAGE in $ENV_FILE}"
: "${MIDASSYS:?Set MIDASSYS in $ENV_FILE}"
ARTIFACT_DIR=$PUBLISHER_ROOT/.artifacts/gaudi-sidecar
SOCKET_PATH=${PIONEER_GAUDI_SOCKET:-/tmp/pioneer-gaudi.sock}
JOB_OPTIONS=${PIONEER_GAUDI_JOB_OPTIONS:-}

show_help() {
    cat <<EOF
Usage: ./extensions/gaudi/scripts/run_sidecar.sh [OPTIONS]

Options:
  --job-options PATH  Publisher-owned Gaudi job-options file
  --socket PATH       Unix socket path (default: /tmp/pioneer-gaudi.sock)
  --image PATH        Apptainer image
  --midas-root PATH   MIDAS installation (default: from .env.local)
  -h, --help          Display this help message

The corresponding PIONEER_GAUDI_* environment variables remain supported.
EOF
}

require_value() {
    if [[ $# -lt 2 || -z "${2:-}" || "${2:-}" == -* ]]; then
        echo "[run_sidecar, ERROR] $1 requires a value" >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --job-options) require_value "$@"; JOB_OPTIONS=$2; shift 2 ;;
        --socket) require_value "$@"; SOCKET_PATH=$2; shift 2 ;;
        --image) require_value "$@"; PIONEER_GAUDI_IMAGE=$2; shift 2 ;;
        --midas-root) require_value "$@"; MIDASSYS=$2; shift 2 ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "[run_sidecar, ERROR] Unknown option: $1" >&2; show_help; exit 2 ;;
    esac
done

if [[ -z "$JOB_OPTIONS" ]]; then
    echo "[run_sidecar, ERROR] Pass --job-options PATH." >&2
    exit 1
fi
JOB_OPTIONS=$(realpath "$JOB_OPTIONS")
JOB_OPTIONS_BIND=()

case "$JOB_OPTIONS" in
    "$PIONEER_SOURCE_DIR"/*)
        CONTAINER_JOB_OPTIONS=/simulation/${JOB_OPTIONS#"$PIONEER_SOURCE_DIR"/}
        ;;
    "$PUBLISHER_ROOT"/*)
        CONTAINER_JOB_OPTIONS=/publisher/${JOB_OPTIONS#"$PUBLISHER_ROOT"/}
        ;;
    *)
        JOB_OPTIONS_DIR=$(dirname "$JOB_OPTIONS")
        CONTAINER_JOB_OPTIONS=/publisher-job/$(basename "$JOB_OPTIONS")
        JOB_OPTIONS_BIND=(--bind "$JOB_OPTIONS_DIR:/publisher-job:ro")
        ;;
esac

if [[ ! -f "$ARTIFACT_DIR/install/lib/libPIONEER_MIDAS_READER.so" ]]; then
    echo "[run_sidecar, ERROR] Sidecar is not built; run ./extensions/gaudi/scripts/build_sidecar.sh first." >&2
    exit 1
fi

mkdir -p "$(dirname "$SOCKET_PATH")"

export PIONEER_GAUDI_SOCKET="$SOCKET_PATH"
apptainer exec --cleanenv \
    --env PIONEER_GAUDI_SOCKET="$SOCKET_PATH" \
    --env PIONEER_GAUDI_JOB_OPTIONS="$CONTAINER_JOB_OPTIONS" \
    --env PIONEER_GAUDI_INCLUDE_HISTOGRAMS="${PIONEER_GAUDI_INCLUDE_HISTOGRAMS:-1}" \
    --env PIONEER_GAUDI_MAX_WAVEFORMS="${PIONEER_GAUDI_MAX_WAVEFORMS:-64}" \
    --env PIONEER_GAUDI_MAX_SAMPLES_PER_WAVEFORM="${PIONEER_GAUDI_MAX_SAMPLES_PER_WAVEFORM:-4096}" \
    --env PIONEER_GAUDI_DQM_EPOCH="${PIONEER_GAUDI_DQM_EPOCH:-online}" \
    --env PIONEER_GAUDI_MUSIP_SIMULATION_GEOMETRY="${PIONEER_GAUDI_MUSIP_SIMULATION_GEOMETRY:-0}" \
    --env MIDASSYS="$MIDASSYS" \
    --bind "$PIONEER_SOURCE_DIR:/simulation" \
    --bind "$PUBLISHER_ROOT:/publisher" \
    --bind "$MIDASSYS:$MIDASSYS" \
    --bind "$(dirname "$SOCKET_PATH"):$(dirname "$SOCKET_PATH")" \
    "${JOB_OPTIONS_BIND[@]}" \
    "$PIONEER_GAUDI_IMAGE" bash -lc '
        set -e
        export ROOTSYS=/software/root/install
        export PATH=/software/root/install/bin:/software/gaudi/install/bin:/software/venv/bin:$PATH
        export LD_LIBRARY_PATH=/publisher/.artifacts/gaudi-sidecar/runtime-lib:/publisher/.artifacts/gaudi-sidecar/install/lib:$MIDASSYS/lib:/software/root/install/lib
        export PYTHONPATH=/publisher/.artifacts/gaudi-sidecar/install/python:/software/root/install/lib:/software/gaudi/install/python:/software/venv/lib/python3.13/site-packages
        export GAUDI_PLUGIN_PATH=/publisher/.artifacts/gaudi-sidecar/install/lib
        source_root=/publisher/.artifacts/gaudi-sidecar/source
        export ROOT_INCLUDE_PATH=$source_root:$source_root/shared:$source_root/shared/headers/include:$source_root/shared/utils/include:$source_root/shared/MonteCarlo/include:$source_root/shared/Reco/include:$source_root/shared/unpacking/include:$source_root/reco_testbeam/pi_midas/data/include:$source_root/reco_testbeam/psm/exp/data/include
        exec gaudirun.py "$PIONEER_GAUDI_JOB_OPTIONS"
    '
