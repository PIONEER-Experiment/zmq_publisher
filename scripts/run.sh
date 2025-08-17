#!/bin/bash

log_enabled=false
debug_enabled=false

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --log       Enable logging output to file"
    echo "  --debug     Run executable under gdb"
    echo "  --help      Show this help message"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)
            log_enabled=true
            shift
            ;;
        --debug)
            debug_enabled=true
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Resolve script directory (scripts/run.sh)
SOURCE="${BASH_SOURCE[0]}"
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Repo root is one level up from scripts directory
repo_root=$(dirname "$script_directory")

# Source environment setup quietly
env_setup="$repo_root/scripts/environment/setup_environment.sh"
if [ -f "$env_setup" ]; then
    source "$env_setup" -q
else
    echo "Warning: environment setup script not found at $env_setup"
fi

# Define executable and log file paths
executable="$repo_root/build/bin/publisher"
log_dir="$repo_root/logs"
mkdir -p "$log_dir"
log_file="$log_dir/midas_publisher.log"

if [ ! -x "$executable" ]; then
    echo "Error: executable not found or not executable: $executable"
    exit 1
fi

# Run executable with or without logging or debugging
if [ "$debug_enabled" = true ]; then
    echo "Running publisher under gdb..."
    gdb --args "$executable"
elif [ "$log_enabled" = true ]; then
    echo "Running publisher with output logged to $log_file"
    "$executable" > "$log_file" 2>&1
else
    echo "Running publisher without logging"
    "$executable"
fi
