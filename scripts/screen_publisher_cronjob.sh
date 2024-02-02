#!/bin/bash

# Default interval in seconds is 1800 seconds (30 minutes)
interval_seconds=1800

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -i, --interval SECONDS Set the interval in seconds (default: 1800 seconds)"
    echo "  -h, --help            Display this help message"
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--interval)
            if [[ -n $2 && $2 -gt 0 ]]; then
                interval_seconds=$2
                shift 2
            else
                echo "Invalid or missing value for -i/--interval option."
                usage
                exit 1
            fi
            ;;
        -h|--help)
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

# Get the directory of the script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Path to publisher_cronjob.sh (replace with realpath)
cronjob_script=$(realpath "$script_directory/publisher_cronjob.sh")

# Run the publisher_cronjob.sh script within a screen session
screen -dmS publisher_cronjob "$cronjob_script" -i "$interval_seconds"
