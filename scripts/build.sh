#!/bin/bash

# Function to display help
display_help() {
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  --overwrite   Overwrite the build directory if it exists"
    echo "  --install     Perform a full installation, including copying binaries"
    echo "  --help        Display this help message"
    exit 0
}

# Parse command-line arguments
overwrite=false
install=false
for arg in "$@"; do
    case $arg in
        --overwrite)
            overwrite=true
            shift
            ;;
        --install)
            install=true
            shift
            ;;
        --help)
            display_help
            ;;
        *)
            echo "Unknown option: $arg"
            display_help
            ;;
    esac
done

# Get the directory of this script (scripts/build.sh)
SOURCE="${BASH_SOURCE[0]}"
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Repo root is one level up from scripts directory
repo_root=$(dirname "$script_directory")

# Source environment setup quietly if the script exists
env_setup="$repo_root/scripts/environment/setup_environment.sh"
if [ -f "$env_setup" ]; then
    source "$env_setup" -q
fi

# Build directory (repo_root/build)
build_directory="$repo_root/build"

# Remove build directory if requested
if $overwrite && [ -d "$build_directory" ]; then
    echo "Removing existing build directory..."
    rm -rf "$build_directory"
fi

# Create build directory if needed
mkdir -p "$build_directory"

# Enter build directory
cd "$build_directory"

# Run cmake (try cmake3 fallback)
if command -v cmake &> /dev/null; then
    cmake ..
elif command -v cmake3 &> /dev/null; then
    cmake3 ..
else
    echo "Error: Neither cmake nor cmake3 is installed."
    exit 1
fi

# Build and install with parallel jobs
make install -j$(nproc)

# If install flag is set, perform additional installation steps
if $install; then
    echo "Performing full installation..."

    # Define install paths - you can adjust these as needed
    BIN_DIR="$repo_root/build/bin"

    if [ -d "$BIN_DIR" ]; then
        echo "Copying binaries to /usr/local/bin..."
        for binfile in "$BIN_DIR"/*; do
            if [ -x "$binfile" ]; then
                echo "Copying $(basename "$binfile")"
                sudo cp "$binfile" /usr/local/bin/
            fi
        done
    else
        echo "Binary directory not found: $BIN_DIR"
    fi

    echo "Full installation finished."
fi

echo "Build and install process complete."
