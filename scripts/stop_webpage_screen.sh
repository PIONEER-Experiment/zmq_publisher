#!/bin/bash

# Send a Ctrl+C to the screen session
screen -S dashboard_webpage -X stuff "^C"

# Wait for a short time (adjust the duration as needed)
sleep 1

# Kill the screen session
screen -X -S dashboard_webpage quit
