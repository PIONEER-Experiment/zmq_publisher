#!/bin/bash

# Get the directory of the script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Default values
overwrite=false

# Function to display usage
usage() {
    echo "Usage: $0 [--overwrite]"
    echo "  --overwrite  Clear the build directory before building"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --overwrite)
            overwrite=true
            shift
            ;;
        *)
            usage
            ;;
    esac
done

# Set variables
BUILD_DIR="$script_directory/../../build"

# Check if the build directory should be cleared
if [ "$overwrite" = true ]; then
    echo "Clearing the build directory..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Navigate to the build directory
cd "$BUILD_DIR" || { echo "Failed to navigate to the build directory"; exit 1; }

# Run CMake and handle fallback to cmake3
echo "Running CMake..."
if ! cmake .. >/dev/null 2>&1; then
    echo "CMake failed, trying cmake3..."
    if ! cmake3 .. >/dev/null 2>&1; then
        echo "Both cmake and cmake3 failed"
        exit 1
    fi
fi

# Build the project using nproc for parallel compilation
echo "Building the project..."
make -j"$(nproc)" >/dev/null 2>&1 || { echo "Build failed"; exit 1; }

# Install the project
echo "Installing the project..."
make install >/dev/null 2>&1 || { echo "Installation failed"; exit 1; }

echo "Build and installation completed successfully."
