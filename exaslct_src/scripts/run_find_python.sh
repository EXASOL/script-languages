#!/bin/bash

set -u

run_find_python_script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
source "$run_find_python_script_dir"/find_python.sh

find_python3_bin
