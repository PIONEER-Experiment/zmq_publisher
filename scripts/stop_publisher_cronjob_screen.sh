#!/bin/bash

# Get the directory of the script
SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do
    DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
    SOURCE=$(readlink "$SOURCE")
    [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
done
script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

# Path to stop_publisher_screen.sh (replace with realpath)
stop_screen_script=$(realpath "$script_directory/stop_publisher_screen.sh")

# Function to stop and kill the publisher_cronjob.sh script
stop_and_kill() {
    # Send a Ctrl+C to the screen session
    screen -S publisher_cronjob -X stuff "^C"

    # Wait for a short time (adjust the duration as needed)
    sleep 1

    # Kill the screen session
    screen -X -S publisher_cronjob quit

    # Call the stop_publisher_screen.sh script with the correct path
    "$stop_screen_script"
}

# Run the stop_and_kill function
stop_and_kill
