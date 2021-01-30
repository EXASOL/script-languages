#!/bin/bash

set -u

exaslct_python_runner_script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
source "$exaslct_python_runner_script_dir"/find_python.sh

function run_exaslct_python () {
    local script_dir="$exaslct_python_runner_script_dir"
    local python3_bin=$(find_python3_bin)
    if [ -n "$python3_bin" ]
    then
        PYTHONPATH="$script_dir"/../.. "$python3_bin" "$script_dir"/../exaslct.py "$@"
        exit $?
    fi
}

run_exaslct_python "$@"

