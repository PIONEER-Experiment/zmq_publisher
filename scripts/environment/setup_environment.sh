#!/bin/bash

# Function to display usage message
usage() {
    echo "Usage: $0 [-a|--add] [-q|--quiet]"
    echo "Flags:"
    echo "  -a, --add     Add MIDASSYS/bin to PATH if MIDASSYS is defined."
    echo "  -q, --quiet   Suppress output messages."
    # Add more flags and descriptions here as needed.
}

# Flags
add_midassys_to_path=false
quiet=false
run_script=true
quit=false

# Parse command line flags
while [[ $# -gt 0 && $quit == false ]]; do
    case "$1" in
        -a|--add)
            add_midassys_to_path=true
            shift
            ;;
        -q|--quiet)
            quiet=true
            shift
            ;;
        -h|--help)
            usage
            quit=true
            run_script=false
            ;;
        *)
            echo "Invalid option: $1" >&2
            usage
            quit=true
            run_script=false
            ;;
    esac
done

if [ "$run_script" = true ]; then

    SOURCE=${BASH_SOURCE[0]}
    while [ -L "$SOURCE" ]; do
        DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
        SOURCE=$(readlink "$SOURCE")
        [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE
    done
    script_directory=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )

    # Read environment variables from the file
    while IFS= read -r line; do
        # Split line into name and value
        name=$(echo "$line" | cut -d'=' -f1)
        value=$(echo "$line" | cut -d'=' -f2-)

        # Export if value non-empty, else warn (only if not quiet)
        if [ -n "$value" ]; then
            export "$name"="$value"
        else
            if [ "$quiet" = false ]; then
                echo "Warning: $name is empty."
            fi
        fi
    done < "$script_directory/environment_variables.txt"

    # Add MIDASSYS/bin to PATH if requested
    if [ "$add_midassys_to_path" = true ]; then
        if [ -n "$MIDASSYS" ]; then
            export PATH="$MIDASSYS/bin:$PATH"
            if [ "$quiet" = false ]; then
                echo "Added MIDASSYS to path."
            fi
        else
            if [ "$quiet" = false ]; then
                echo "Warning: MIDASSYS is not set. Cannot add to PATH."
            fi
        fi
    fi

    # Add $ZEROMQ_ROOT/lib to LD_LIBRARY_PATH
    if [ -n "$ZEROMQ_ROOT" ]; then
        export LD_LIBRARY_PATH="$ZEROMQ_ROOT/lib:$LD_LIBRARY_PATH"
        if [ "$quiet" = false ]; then
            echo "Added $(realpath "$ZEROMQ_ROOT/lib") to LD_LIBRARY_PATH."
        fi
    else
        if [ "$quiet" = false ]; then
            echo "Warning: ZEROMQ_ROOT is not set. Cannot add to LD_LIBRARY_PATH."
        fi
    fi

    # Echo environment variables (only if not quiet)
    if [ "$quiet" = false ]; then
        echo "MIDAS_EXPT_NAME environment variable set to: $MIDAS_EXPT_NAME"
        echo "MIDASSYS environment variable set to: $MIDASSYS"
        echo "MIDAS_EXPTAB environment variable set to: $MIDAS_EXPTAB"
        echo "ZEROMQ_ROOT environment variable set to: $ZEROMQ_ROOT"
        echo "CPPZMQ_ROOT environment variable set to: $CPPZMQ_ROOT"
    fi

fi
