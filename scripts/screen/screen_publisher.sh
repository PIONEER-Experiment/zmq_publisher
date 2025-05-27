#!/bin/bash

# Default log status is false (logging disabled)
log_enabled=false

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --log       Enable output logging"
    echo "  --help      Display this help message"
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)
            log_enabled=true
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

# Get the absolute root directory of the repo (two levels up from this script)
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
script_path=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
repo_root=$(dirname "$(dirname "$script_path")")

# Define the name for the screen session
screen_session_name="midas_publisher"

# Path to the setup_environment.sh script
setup_script="$repo_root/scripts/environment/setup_environment.sh"

# Path to the publisher binary (assuming you build into build/bin/)
publisher_binary="$repo_root/build/bin/publisher"

# Define a log file directory
mkdir -p "$repo_root/logs"
log_file="$repo_root/logs/midas_publisher.log"

# Kill all existing screen sessions with this name
screen -ls | grep -oE "[0-9]+\.$screen_session_name" | while read -r session; do
    echo "Killing existing screen session '$session'..."
    screen -S "$session" -X quit
done

# Create a new screen session
screen -S "$screen_session_name" -d -m

# Source the environment setup script inside screen
screen -S "$screen_session_name" -X stuff $"source $setup_script\n"

# Run the publisher binary with or without logging
if [ "$log_enabled" = true ]; then
    screen -S "$screen_session_name" -X stuff $"echo 'Output is logged to $log_file.'\n"
    screen -S "$screen_session_name" -X stuff "$publisher_binary > $log_file 2>&1\n"
    echo "Started '$publisher_binary' inside a new screen session. Output is logged to $log_file."
else
    screen -S "$screen_session_name" -X stuff "$publisher_binary\n"
    echo "Started '$publisher_binary' inside a new screen session without logging."
fi
