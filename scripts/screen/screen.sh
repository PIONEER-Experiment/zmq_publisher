#!/bin/bash

# Get absolute paths
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
BASE_DIR="$(realpath "$SCRIPT_DIR/../..")"
ORIG_DIR="$(pwd)"

# Constants
PROC_NAME="publisher"
SCREEN_NAME="publisher_gm2"
EXECUTABLE="$BASE_DIR/build/bin/$PROC_NAME"
UNPACKER_PATH="$BASE_DIR/build/_deps/unpacker-src"
STOP_ONLY=false
EXE_ARGS=()

# Help text
show_help() {
    echo "Usage: ./screen.sh [options] [-- <args>]"
    echo
    echo "Options:"
    echo "  -h, --help         Show this help message"
    echo "  --stop             Stop the screen session and exit"
    echo
    echo "Arguments:"
    echo "  -- <args>          Arguments passed to the '$PROC_NAME' executable"
}

# Parse command-line args
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) show_help; exit 0 ;;
        --stop) STOP_ONLY=true; shift ;;
        --) shift; EXE_ARGS+=("$@"); break ;;
        -*)
            echo "[screen.sh, ERROR] Unknown option: $1"
            show_help; exit 1 ;;
        *)
            echo "[screen.sh, WARN] Ignoring unexpected argument: $1"
            shift ;;
    esac
done

# Function to stop existing session gracefully
stop_screen_session() {
    if screen -list | grep -q "\.${PROC_NAME}"; then
        echo "[screen.sh, INFO] Sending Ctrl+C to '$PROC_NAME'..."
        screen -S "$PROC_NAME" -X stuff $'\003'

        for i in {1..10}; do
            sleep 0.5
            if ! screen -list | grep -q "\.${PROC_NAME}"; then
                echo "[screen.sh, OK] Session stopped gracefully."
                return
            fi
        done

        echo "[screen.sh, WARN] Graceful stop failed. Forcing quit..."
        screen -S "$PROC_NAME" -X quit
        sleep 0.2
    fi
}

# If --stop, just stop and exit
if [ "$STOP_ONLY" = true ]; then
    stop_screen_session
    exit 0
fi

# Always stop any running instance before launch
stop_screen_session

# Check for binary
if [ ! -f "$EXECUTABLE" ]; then
    echo "[screen.sh, ERROR] Executable not found at: $EXECUTABLE"
    echo "Did you forget to build it?"
    exit 1
fi

# Set environment
export UNPACKER_PATH="$UNPACKER_PATH"

# Launch new session
echo "[screen.sh, INFO] Launching new screen session '$SCREEN_NAME'..."
cd "$BASE_DIR" || exit 1

screen -S "$SCREEN_NAME" -dm bash -c "
    echo '[screen.sh] Starting $EXECUTABLE with args: ${EXE_ARGS[*]}';
    \"$EXECUTABLE\" ${EXE_ARGS[*]};
    echo '[screen.sh] Process exited. Press ENTER to close...';
    read -r
"

cd "$ORIG_DIR"
