#!/usr/bin/env bash

set -euo pipefail

# Resolve absolute paths
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE_DIR=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$BASE_DIR/build"
ENV_FILE="$SCRIPT_DIR/environment/.env"

# Default flags
OVERWRITE=false
JOBS_ARG="-j"  # Use all processors
WITH_MIDAS=false

# Help message
show_help() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo
    echo "Options:"
    echo "  -o, --overwrite           Remove existing build directory before building"
    echo "  -j, --jobs <number>       Specify number of build jobs (default: all available)"
    echo "  --with-midas              Build the optional MIDAS extension"
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
if [ "$WITH_MIDAS" = true ]; then
    if [[ -f "$ENV_FILE" ]]; then
        # shellcheck disable=SC1090
        source "$ENV_FILE"
    fi
    if [[ -z "${MIDASSYS:-}" ]]; then
        echo "[build.sh, ERROR] MIDASSYS is not set. Run './scripts/setup_env.sh --with-midas'."
        exit 1
    fi
    CMAKE_OPTIONS+=("-DZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION=ON")
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
