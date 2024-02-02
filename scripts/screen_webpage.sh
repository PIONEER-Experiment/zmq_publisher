#!/bin/bash

# Default log status is true
log_enabled=false

# Function to display usage
usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --log       Enables output logging"
    echo "  --help      Display this help message"
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)
            log_enabled=false
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

# Get the directory of the script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Define the name for the screen session
screen_session_name="dashboard_webpage"

# Path to the setup_environment.sh script (replace with realpath)
setup_script=$(realpath "$script_directory/../environment_setup/setup_environment.sh")

# Path to main.py (replace with realpath)
main_script=$(realpath "$script_directory/../dashboard_webpage/main.py")

# Define a log file for output (replace with realpath)
log_file=$(realpath "$script_directory/../dashboard_webpage/dashboard.log")

# Kill all instances of the screen session
screen -ls | grep -oE "[0-9]+\.$screen_session_name" | while read -r session; do
    echo "Killing existing screen session '$session'..."
    screen -S "$session" -X quit
done

# Create a new screen session
screen -S "$screen_session_name" -d -m

# Source the setup_environment.sh script
screen -S "$screen_session_name" -X stuff $"source $setup_script\n"

# Run your desired command with or without logging
if [ "$log_enabled" = true ]; then
    screen -S "$screen_session_name" -X stuff $"echo 'Output is logged to $log_file.'\n"
    screen -S "$screen_session_name" -X stuff $"python3 $main_script > $log_file 2>&1\n"
    echo "Started 'python3 $main_script' inside a new screen session with environment variables set. Output is logged to $log_file."
else
    screen -S "$screen_session_name" -X stuff $"python3 $main_script\n"
fi
