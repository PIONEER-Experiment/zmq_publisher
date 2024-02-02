#!/bin/bash

# Default interval in seconds is 1800 seconds (30 minutes)
interval_seconds=1800

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -h, --help          Display this help message"
    echo "  -i, --interval SECONDS Set the interval in seconds (default: 1800 seconds)"
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

# Path to stop_publisher.sh (replace with realpath)
stop_script=$(realpath "$script_directory/stop_publisher_screen.sh")

# Path to start_publisher.sh (replace with realpath)
start_script=$(realpath "$script_directory/screen_publisher.sh")

# Function to stop and start the publisher script
stop_and_start() {
    # Stop the publisher script
    $stop_script

    # Wait for a couple of seconds
    sleep 2

    # Start the publisher script
    $start_script

    # Print a message with the current date and time
    echo "Publisher script stopped and started at $(date)"
}

# Run the stop_and_start function initially
stop_and_start

# Run the stop_and_start function at regular intervals
while true; do
    sleep "$interval_seconds"
    stop_and_start
done
