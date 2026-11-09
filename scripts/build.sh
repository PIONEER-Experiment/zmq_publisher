#!/usr/bin/env bash

set -euo pipefail

# Resolve absolute paths
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE_DIR=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$BASE_DIR/build"
ENV_FILE="$SCRIPT_DIR/environment/.env.local"

# Default flags
OVERWRITE=false
JOBS_ARG="-j"  # Use all processors
WITH_MIDAS=false
WITH_GAUDI=false
WITH_SIMULATORS=false

# Help message
show_help() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo
    echo "Options:"
    echo "  -o, --overwrite           Remove existing build directory before building"
    echo "  -j, --jobs <number>       Specify number of build jobs (default: all available)"
    echo "  --with-midas              Build the optional MIDAS extension"
    echo "  --with-gaudi              Build the optional MIDAS-to-Gaudi extension"
    echo "  --with-simulators         Build the MIDAS detector simulator frontends"
    echo "  -h, --help                Display this help message"
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -o|--overwrite)
            OVERWRITE=true
            shift
            ;;
        -j|--jobs)
            if [[ $# -gt 1 && -n "$2" && "$2" != -* ]]; then
                JOBS_ARG="-j$2"
                shift 2
            else
                JOBS_ARG="-j"
                shift
            fi
            ;;
        --with-midas)
            WITH_MIDAS=true
            shift
            ;;
        --with-gaudi)
            WITH_GAUDI=true
            shift
            ;;
        --with-simulators)
            WITH_SIMULATORS=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "[build.sh, ERROR] Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Optionally clean build
if [ "$OVERWRITE" = true ]; then
    echo "[build.sh] Removing previous build directory: $BUILD_DIR"
    cmake -E remove_directory "$BUILD_DIR"
fi

# Configure and build
echo "[build.sh] Running cmake in: $BUILD_DIR"
CMAKE_OPTIONS=()
CMAKE_OPTIONS+=(
    "-DZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION=OFF"
    "-DZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION=OFF"
)
if [ "$WITH_MIDAS" = true ] || [ "$WITH_GAUDI" = true ] || [ "$WITH_SIMULATORS" = true ]; then
    if [[ -f "$ENV_FILE" ]]; then
        # shellcheck disable=SC1090
        source "$ENV_FILE"
    fi
    if [[ -z "${MIDASSYS:-}" ]]; then
        echo "[build.sh, ERROR] MIDASSYS is not set in scripts/environment/.env.local."
        echo "[build.sh, ERROR] Edit that file and run './scripts/setup_env.sh --with-midas'."
        exit 1
    fi
    MIDAS_RECEIVER_SOURCE=${MIDAS_RECEIVER_SOURCE:-$BASE_DIR/../midas_receiver}
    if [[ -f "$MIDAS_RECEIVER_SOURCE/CMakeLists.txt" ]]; then
        CMAKE_OPTIONS+=("-DCPM_midas_receiver_SOURCE=$MIDAS_RECEIVER_SOURCE")
    fi
fi
if [ "$WITH_MIDAS" = true ]; then
    CMAKE_OPTIONS+=("-DZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION=ON")
fi
if [ "$WITH_GAUDI" = true ]; then
    CMAKE_OPTIONS+=("-DZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION=ON")
fi
if [ "$WITH_SIMULATORS" = true ]; then
    CMAKE_OPTIONS+=("-DZMQ_PUBLISHER_BUILD_RATE_TESTS=ON")
else
    CMAKE_OPTIONS+=("-DZMQ_PUBLISHER_BUILD_RATE_TESTS=OFF")
fi
cmake -S "$BASE_DIR" -B "$BUILD_DIR" "${CMAKE_OPTIONS[@]}"

echo "[build.sh] Building with $JOBS_ARG"
if [ "$JOBS_ARG" = "-j" ]; then
    cmake --build "$BUILD_DIR" --parallel
else
    cmake --build "$BUILD_DIR" --parallel "${JOBS_ARG#-j}"
fi

echo "[build.sh] Build complete."
echo "[build.sh] Executables are in: $BUILD_DIR/bin/"
echo "[build.sh] Libraries are in: $BUILD_DIR/lib/"
