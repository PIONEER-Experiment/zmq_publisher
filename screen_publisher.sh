#!/bin/bash

# Get the directory of the script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Define the name for the screen session
screen_session_name="midas_publisher"

# Path to the setup_environment.sh script
setup_script="$script_directory/environment_setup/setup_environment.sh"

# Kill all instances of the screen session
screen -ls | grep -oE "[0-9]+\.$screen_session_name" | while read -r session; do
    echo "Killing existing screen session '$session'..."
    screen -S "$session" -X quit
done

# Create a new screen session
screen -S "$screen_session_name" -d -m

# Source the setup_environment.sh script
screen -S "$screen_session_name" -X stuff $"source $setup_script\n"

# Run your desired command
screen -S "$screen_session_name" -X stuff $'./publisher\n'

echo "Started './publisher' inside a new screen session with environment variables set."
