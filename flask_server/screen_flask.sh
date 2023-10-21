#!/bin/bash

# Define the name of the screen session for the old server
OLD_SESSION_NAME="flask_screen"

# Define the name of the screen session for the new server
NEW_SESSION_NAME="flask_screen"

# Define the Python script you want to run
PYTHON_SCRIPT="data_pipeline.py"

# Check if the old server screen session is running
if screen -ls | grep -q "$OLD_SESSION_NAME"; then
    # Send a Ctrl+C to the old server (SIGINT to stop it)
    screen -S "$OLD_SESSION_NAME" -X stuff $'\003'
    screen -S "$OLD_SESSION_NAME" -X stuff $'\003'
    # Wait a moment to allow the old server to terminate
    sleep 1
    # Kill the old screen session
    screen -S "$OLD_SESSION_NAME" -X quit
fi

# Create a new screen session for the new server
screen -dmS "$NEW_SESSION_NAME"

# Send the Python script to the new screen session
screen -S "$NEW_SESSION_NAME" -X stuff "python3 $PYTHON_SCRIPT^M"
