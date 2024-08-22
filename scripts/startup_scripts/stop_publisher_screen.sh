#!/bin/bash

# Send a Ctrl+C to the screen session
screen -S midas_publisher -X stuff "^C"

# Wait for a short time (adjust the duration as needed)
sleep 1

# Kill the screen session
screen -X -S midas_publisher quit
