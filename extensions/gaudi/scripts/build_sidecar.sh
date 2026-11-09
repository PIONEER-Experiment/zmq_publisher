#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(dirname "$(realpath "$0")")
PUBLISHER_ROOT=$(realpath "$SCRIPT_DIR/../../..")
ENV_FILE=$PUBLISHER_ROOT/scripts/environment/.env.local
ARTIFACT_DIR=$PUBLISHER_ROOT/.artifacts/gaudi-sidecar
JOBS=${JOBS:-$(nproc)}
SHARED_REVISION=${PIONEER_SHARED_REVISION:-origin/develop-testbeam}

show_help() {
    cat <<EOF
Usage: ./extensions/gaudi/scripts/build_sidecar.sh [OPTIONS]

Options:
  --jobs N     Parallel build jobs (default: detected CPU count)
  --shared-revision REF
               Shared revision copied into the isolated build overlay
               (default: origin/develop-testbeam)
  -h, --help   Display this help message

Machine-specific paths are read from scripts/environment/.env.local.
EOF
}

require_value() {
    if [[ $# -lt 2 || -z "${2:-}" || "${2:-}" == -* ]]; then
        echo "[build_sidecar, ERROR] $1 requires a value" >&2
        exit 2
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --jobs) require_value "$@"; JOBS=$2; shift 2 ;;
        --shared-revision) require_value "$@"; SHARED_REVISION=$2; shift 2 ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "[build_sidecar, ERROR] Unknown option: $1" >&2; show_help; exit 2 ;;
    esac
done

if [[ ! -f "$ENV_FILE" ]]; then
    echo "[build_sidecar, ERROR] Run ./scripts/setup_env.sh --with-gaudi first." >&2
    exit 1
fi
# shellcheck disable=SC1090
source "$ENV_FILE"
: "${PIONEER_SOURCE_DIR:?Set PIONEER_SOURCE_DIR in $ENV_FILE}"
: "${PIONEER_GAUDI_IMAGE:?Set PIONEER_GAUDI_IMAGE in $ENV_FILE}"
: "${MIDASSYS:?Set MIDASSYS in $ENV_FILE}"

if [[ ! "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "[build_sidecar, ERROR] --jobs must be a positive integer" >&2
    exit 2
fi

for path in "$PIONEER_GAUDI_IMAGE" "$MIDASSYS"; do
    if [[ ! -e "$path" ]]; then
        echo "[build_sidecar, ERROR] Required path does not exist: $path" >&2
        exit 1
    fi
done
SHARED_REPOSITORY=$PIONEER_SOURCE_DIR/shared
if ! git -C "$SHARED_REPOSITORY" cat-file -e "$SHARED_REVISION^{commit}" 2>/dev/null; then
    echo "[build_sidecar, ERROR] Shared revision is unavailable: $SHARED_REVISION" >&2
    echo "Fetch it in $SHARED_REPOSITORY or pass --shared-revision REF." >&2
    exit 1
fi

mkdir -p "$ARTIFACT_DIR"
SHARED_SOURCE=$ARTIFACT_DIR/shared-source
rm -rf "$SHARED_SOURCE"
mkdir -p "$SHARED_SOURCE"
git -C "$SHARED_REPOSITORY" archive "$SHARED_REVISION" | tar -x -C "$SHARED_SOURCE"
if [[ ! -f "$SHARED_SOURCE/gaudi/services/IPIConditionsSvc.h" ]]; then
    echo "[build_sidecar, ERROR] Shared revision is incompatible: $SHARED_REVISION" >&2
    exit 1
fi

apptainer exec --cleanenv \
    --bind "$PIONEER_SOURCE_DIR:/simulation" \
    --bind "$PUBLISHER_ROOT:/publisher" \
    --bind "$MIDASSYS:$MIDASSYS" \
    "$PIONEER_GAUDI_IMAGE" bash -lc "
        set -e
        export ROOTSYS=/software/root/install
        export PATH=/software/root/install/bin:/software/gaudi/install/bin:/software/venv/bin:\$PATH
        export LD_LIBRARY_PATH=/software/root/install/lib:/software/gaudi/install/lib:/software/clhep/install/lib:/software/genfit/install/lib:/software/arrow/install/lib
        export CMAKE_PREFIX_PATH=/software/root/install:/software/gaudi/install:/software/clhep/install:/software/genfit/install:/software/arrow/install
        export PYTHONPATH=/software/root/install/lib:/software/gaudi/install/python:/software/venv/lib/python3.13/site-packages
        export PIONEERSYS=/simulation
        export MIDASSYS=$MIDASSYS
        # ROOT dictionaries retain the paths of the headers from which they
        # were generated. Keep this overlay below the bind-mounted artifact
        # directory so those paths remain valid in every sidecar process.
        overlay=/publisher/.artifacts/gaudi-sidecar/source
        rm -rf \"\$overlay\"
        mkdir -p \"\$overlay/reco_testbeam\"
        for source in /simulation/*; do
            name=\"\${source##*/}\"
            if [[ \"\$name\" != reco_testbeam && \"\$name\" != shared ]]; then
                ln -s \"\$source\" \"\$overlay/\$name\"
            fi
        done
        ln -s /publisher/.artifacts/gaudi-sidecar/shared-source \"\$overlay/shared\"
        if [[ -e /simulation/.git ]]; then
            ln -s /simulation/.git \"\$overlay/.git\"
        fi
        for source in /simulation/reco_testbeam/*; do
            name=\"\${source##*/}\"
            if [[ \"\$name\" != CMakeLists.txt ]]; then
                ln -s \"\$source\" \"\$overlay/reco_testbeam/\$name\"
            fi
        done
        cp /simulation/reco_testbeam/CMakeLists.txt \"\$overlay/reco_testbeam/CMakeLists.txt\"
        printf '\nadd_subdirectory(online_dqm)\n' >> \"\$overlay/reco_testbeam/CMakeLists.txt\"
        cmake -S \"\$overlay\" -B /publisher/.artifacts/gaudi-sidecar/build \
            -DCMAKE_INSTALL_PREFIX=/publisher/.artifacts/gaudi-sidecar/install \
            -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=/publisher/extensions/gaudi/cmake/bootstrap.cmake \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DBUILD_MC=OFF -DBUILD_DER=OFF -DBUILD_REC=ON \
            -DBUILD_TESTBEAM=ON -DBUILD_TESTING=OFF
        cmake --build /publisher/.artifacts/gaudi-sidecar/build --parallel $JOBS \
            --target PiGaudiSharedSvc PIONEER_MIDAS_READER PIONEER_TB_ONLINE_DQM
        cmake -E remove_directory /publisher/.artifacts/gaudi-sidecar/install
        cmake -E make_directory /publisher/.artifacts/gaudi-sidecar/install/lib
        cmake -E make_directory /publisher/.artifacts/gaudi-sidecar/install/python
        cp -a /publisher/.artifacts/gaudi-sidecar/build/shared/*.so \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/shared/*.components \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/shared/*_rdict.pcm \
            /publisher/.artifacts/gaudi-sidecar/build/shared/*.rootmap \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/*.so \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/*_rdict.pcm \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/*.rootmap \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/libPIONEER_MIDAS_READER.so \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/libPIONEER_MIDAS_DATA.so \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/libPIONEER_MIDAS_DATA_rdict.pcm \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/libPIONEER_MIDAS_DATA.rootmap \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/PIONEER_MIDAS_READER.components \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/online_dqm/libPIONEER_TB_ONLINE_DQM.so \
            /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/online_dqm/PIONEER_TB_ONLINE_DQM.components \
            /publisher/.artifacts/gaudi-sidecar/install/lib/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/shared/genConfDir/. \
            /publisher/.artifacts/gaudi-sidecar/install/python/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/pi_midas/genConfDir/. \
            /publisher/.artifacts/gaudi-sidecar/install/python/
        cp -a /publisher/.artifacts/gaudi-sidecar/build/reco_testbeam/online_dqm/genConfDir/. \
            /publisher/.artifacts/gaudi-sidecar/install/python/
        cmake -E remove_directory /publisher/.artifacts/gaudi-sidecar/runtime-lib
        cmake -E make_directory /publisher/.artifacts/gaudi-sidecar/runtime-lib
        cp -a /software/gaudi/install/lib/. /publisher/.artifacts/gaudi-sidecar/runtime-lib/
    "

echo "[build_sidecar] Installed into $ARTIFACT_DIR/install"
