#!/bin/bash

set -u

find_python_script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

source "$find_python_script_dir"/print_utils.sh

function get_supported_python_version_regex () {
  echo "3\.[678]\.[0-9]"
}

print_install_instructions_for_python3_bin () {
    local os_name=$(cat /etc/os-release | grep "^ID=" | cut -f2 -d "=")
    local os_version=$(cat /etc/os-release | grep "^VERSION_ID=" | cut -f2 -d "=")
    # os_name=centos
    # os_version="7"
    (
        case "$os_name" in
            fedora)
                if [ "$os_version" -ge 18 ]
                then
                    echo "You are using Fedora 18 or later, please install python3 with the following command, or ask your adminestrator to install python3:"
                    echo "  sudo dnf install <python_version>"
                    echo "for the following options for the python_version:"
                    echo "  - python36"
                    echo "  - python37"
                    echo "  - python38"
                else 
                    echo "You are using Fedora 17 or earlier, please ask your adminestrator to install python3:"
                fi
                ;;  
            ubuntu)
                local os_major_version=$(echo "$os_version" | cut -f1 -d ".")
                if [ "$os_major_version" -ge 18 ]
                then
                    echo "You are using Ubuntu 18 or later, please install python3 with the following command, or ask your adminestrator to install python3:"
                    echo "  sudo apt-get install python3"
                else
                    echo "You are using Ubuntu 17 or earlier, please ask your adminestrator to install python3:"
                fi
                ;;
             
            centos)
                if [ "$os_version" -ge 7 ]
                then
                    echo "You are using Centos 7 or later, please install python3 with the following command, or ask your adminestrator to install python3:"
                    echo "  yum install python3"
                else
                    echo "You are using Centos 6 or earlier, please ask your adminestrator to install python3:"
                fi
                ;;
            *)
                echo "Please follow the installation procedure for your operating system to install python3, or ask your adminestrator to install python3."
       
        esac
    ) >&2
}

check_python3_bin_version () {
    local python3_bin="$1"
    local python3_bin_version=$("$python3_bin" --version | cut -f2 -d " ")
    local supported_python_version_regex=$(get_supported_python_version_regex)
    local check_name="python3 binary version"
    if [[ "$python3_bin_version" =~ $supported_python_version_regex ]]
    then
      print_ok "$check_name" "'$python3_bin_version' supported"
      return 0
    else
      print_failed "$check_name" "'$python3_bin_version' not supported, please install python3 (supported version 3.6, 3.7, 3.8)"
      print_install_instructions_for_python3_bin
      return 1
    fi
}

check_python3_bin_available () {
    local python3_bin="$1"
    local python3_bin_test_run=$("$python3_bin" --version 2>&1)
    local check_name="python3 binary is executable"
    if [ -n "$python3_bin_test_run" ]
    then
        print_ok "$check_name" ""
        return 0
    else
        print_failed "$check_name" "'$python3_bin' can't be executed, got following error\n $python3_bin_test_run"
        print_install_instructions_for_python3_bin
        return 1
    fi
}

check_python3_bin () {
    local python3_bin="$1"
    if check_python3_bin_available "$python3_bin"
    then
        if check_python3_bin_version "$python3_bin"
        then
            return 0
        else
            return 1
        fi
    else
        return 1
    fi
}


find_python3_bin () {
    local check_name="python binary"
    if [ ! "${EXASLCT_PYTHON3_BIN+x}" = "x" ]
    then
      python3_bin=$(command -v python3)
      if [ -n "$python3_bin" ]
      then
        print_ok "$check_name" "found at '$python3_bin'"
      else
        print_failed "$check_name" "not found, please install python3 (supported version 3.6, 3.7, 3.8)"
        return 1
      fi
    else
      local python3_bin=${EXASLCT_PYTHON3_BIN}
      print_ok "$check_name" "defined in \$EXASLCT_PYTHON3_BIN as '$python3_bin'"
    fi
    if check_python3_bin "$python3_bin"
    then
      echo "$python3_bin"
      return 0
    else
      return 1
    fi
}
