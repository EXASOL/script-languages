#!/bin/bash

# This script acts as the start of exaslct the script language container build tool
# It tries to discover if pipenv is already installed or tries to install it if it not exists.
# After that it creates the virtual environment and install all necessary dependencies for exaslct.
# In the end it runs exaslct_src/exaslct.py in the virtual environment.

COMMAND_LINE_ARGS="$@"
SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

run_python () {
    export PYTHONPATH="$SCRIPT_DIR"
    python3 "$SCRIPT_DIR/exaslct_src/exaslct.py" "${COMMAND_LINE_ARGS[@]}"
    exit $?
}

run_pipenv () {
    $PIPENV_BIN run python3 "$SCRIPT_DIR/exaslct_python_runner.sh" "${COMMAND_LINE_ARGS[@]}"
    exit $?
}

source pipenv_utils.sh
discover_pipenv
init_pipenv "$PIPENV_BIN"
if [ -n "$PIPENV_BIN" ]
then
  run_pipenv
else
  echo "Could not find pipenv!"
  exit 1
fi
