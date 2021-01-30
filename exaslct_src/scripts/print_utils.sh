#!/bin/bash

set -u

print_ok () {
    local check_name="$1"
    shift
    local message="$*"
    echo "[OK]     $check_name: $message" >&2
}

print_failed () {
    local check_name="$1"
    shift
    local message="$*"
    echo "[FAILED] $check_name: $message" >&2
}
