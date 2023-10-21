#!/bin/bash

# Start Redis in a detached screen session
screen -dmS redis-session redis-server

# Optional: Display a message indicating that Redis is running in a screen session
echo "Redis is running in a screen session. You can attach using 'screen -r redis-session'."