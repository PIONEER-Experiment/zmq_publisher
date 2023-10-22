#!/bin/bash

# Function to display usage message
usage() {
    echo "Usage: $0 [-a|--add]"
    echo "Flags:"
    echo "  -a, --add   Add MIDASSYS/bin to PATH if MIDASSYS is defined."
    # Add more flags and descriptions here as needed.
}

# Flags
add_midassys_to_path=false
run_script=true  # Set to true by default
quit=false  # Initialize quit to false

# Parse command line flags
while [[ $# -gt 0 && $quit == false ]]; do
    case "$1" in
        -a|--add)
            add_midassys_to_path=true
            shift
            ;;
        -h|--help)
            usage
            quit=true  # Set quit to true when invalid option is encountered
            run_script=false  # Set to false when invalid option is encountered
            ;;
        *)
            echo "Invalid option: $1" >&2
            usage
            quit=true  # Set quit to true when invalid option is encountered
            run_script=false  # Set to false when invalid option is encountered
            ;;
    esac
done

# Check if run_script is true before executing the main functionality
if [ "$run_script" = true ]; then
    # Check if enable scripts exist and source them, otherwise, warn the user
    if [ -f "/opt/rh/devtoolset-11/enable" ]; then
        source /opt/rh/devtoolset-11/enable
    else
        echo "Warning: devtoolset-11 enable script not found."
    fi

    # Read environment variables from the file
    while IFS= read -r line; do
        # Split the line into variable name and value
        name=$(echo "$line" | cut -d'=' -f1)
        value=$(echo "$line" | cut -d'=' -f2-)

        # Check if the value is empty, and if not, export the variable
        if [ -n "$value" ]; then
            export "$name"="$value"
        else
            echo "Warning: $name is empty."
        fi
    done < environment_variables.txt


    # Check if the flag for adding MIDASSYS to PATH is present
    if [ "$add_midassys_to_path" = true ]; then
        if [ -n "$MIDASSYS" ]; then
            export PATH="$MIDASSYS/bin:$PATH"
            echo "Added MIDASSYS to path."
        else
            echo "Warning: MIDASSYS is not set. Cannot add to PATH."
        fi
    fi

    # Specify the location of ROOT and the path to thisroot.sh
    ROOT_SETUP_SCRIPT=$ROOT_ROOT/root_install/bin/thisroot.sh

    # Check if the thisroot.sh script exists
    if [ -f "$ROOT_SETUP_SCRIPT" ]; then
        source "$ROOT_SETUP_SCRIPT"
        echo "Sourced thisroot.sh from $ROOT_SETUP_SCRIPT"
    else
        echo "Error: thisroot.sh not found at $ROOT_SETUP_SCRIPT"
    fi

    # Add the LD_LIBRARY_PATH line with $UNPACKING_ROOT
    if [ -n "$UNPACKING_ROOT" ]; then
        export LD_LIBRARY_PATH="$UNPACKING_ROOT/lib:$LD_LIBRARY_PATH"
        echo "Added $(realpath $UNPACKING_ROOT/lib) to LD_LIBRARY_PATH."
    else
        echo "Warning: UNPACKING_ROOT is not set. Cannot add to LD_LIBRARY_PATH."
    fi

    # Add the LD_LIBRARY_PATH line with $UNPACKING_ROOT
    if [ -n "$ZEROMQ_ROOT" ]; then
        export LD_LIBRARY_PATH="$ZEROMQ_ROOT/lib:$LD_LIBRARY_PATH"
        echo "Added $(realpath $ZEROMQ_ROOT/lib) to LD_LIBRARY_PATH."
    else
        echo "Warning: ZEROMQ_ROOT is not set. Cannot add to LD_LIBRARY_PATH."
    fi


    # Echo the environment variables
    echo "MIDAS_EXPT_NAME environment variable set to: $MIDAS_EXPT_NAME"
    echo "MIDASSYS environment variable set to: $MIDASSYS"
    echo "MIDAS_EXPTAB environment variable set to: $MIDAS_EXPTAB"
    echo "ROOT_ROOT environment variable set to: $ROOT_ROOT"
    echo "UNPACKING_ROOT environment variable set to: $UNPACKING_ROOT"
    echo "BOOST_1_70_0_ROOT environment variable set to: $BOOST_1_70_0_ROOT"
    echo "ZEROMQ_ROOT environment variable set to: $ZEROMQ_ROOT"
    echo "CPPZMQ_ROOT environment variable set to: $CPPZMQ_ROOT"
fi
