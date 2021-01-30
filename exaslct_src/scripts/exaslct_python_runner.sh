#!/bin/bash


COMMAND_LINE_ARGS="$@"
SCRIPT_DIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
SUPPORTED_PYTHON_VERSION_REGEX="3\.[678]\.[0-9]"

run_python () {
    PYTHONPATH="$SCRIPT_DIR/../.." PYTHON3_BIN "$SCRIPT_DIR/../exaslct.py" "${COMMAND_LINE_ARGS[@]}"
    exit $?
}

check_python3_bin_version () {
    local PYTHON_BIN_VERSION=$("$PYTHON3_BIN" --version | cut -f2 -d " ")
    if [[ "$PYTHON_BIN_VERSION" =~ $SUPPORTED_PYTHON_VERSION_REGEX ]]
    then
      echo "[OK] python3 binary version '$PYTHON_BIN_VERSION' supported"
      return 0
    else
      echo "[FAILED] python3 binary version '$PYTHON_BIN_VERSION' not supported, please install python3 (supported version 3.6, 3.7, 3.8)"
      exit 1
    fi
}

check_python3_bin_available () {
  local PYTHON3_BIN_TEST_RUN=$("$PYTHON3_BIN" --version 2>1)
  if [ -n "$PYTHON3_BIN_TEST_RUN" ]
  then
      echo "[OK] python3 binary '$PYTHON3_BIN' can be executable"
      return 0
  else
      echo "[FAILED] python3 binary '$PYTHON3_BIN' can't be executed, got following error\n $PYTHON3_BIN_TEST_RUN"
      exit 1
  fi
}

check_python3_bin () {
    check_python3_bin_available
    check_python3_bin_version
}

print_install_instructions_for_python3_bin () {
  local OS_NAME=$(cat /etc/os-release | grep "^ID=" | cut -f2 -d "=")
  local OS_VERSION=$(cat /etc/os-release | grep "^VERSION_ID=" | cut -f2 -d "=")
  OS_NAME=centos
  OS_VERSION="7"
  case "$OS_NAME" in
        fedora)
            if [ "$OS_VERSION" -ge 18 ]
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
            local OS_MAJOR_VERSION=$(echo "$OS_VERSION" | cut -f1 -d ".")
            if [ "$OS_MAJOR_VERSION" -ge 18 ]
            then
              echo "You are using Ubuntu 18 or later, please install python3 with the following command, or ask your adminestrator to install python3:"
              echo "  sudo apt-get install python3"
            else
              echo "You are using Ubuntu 17 or earlier, please ask your adminestrator to install python3:"
            fi
            ;;
         
        centos)
            echo centos
            if [ "$OS_VERSION" -ge 7 ]
            then
              echo "You are using Centos 7 or later, please install python3 with the following command, or ask your adminestrator to install python3:"
              echo "  yum install python3"
            else
              echo "You are using Centos 6 or earlier, please ask your adminestrator to install python3:"
            fi
            ;;
        *)
            echo "Please follow the installation procedure for your operating system to install python3, or ask your adminestrator to install python3."
            exit 1
 
esac
}

find_python3_bin () {
    if [ -z "$PYTHON3_BIN" ]
    then
      PYTHON3_BIN=$(command -v python3)
      if [ -n "$PYTHON3_BIN" ]
      then
        echo "[OK] python3 binary found at '$PYTHON3_BIN'"
        check_python3_bin
        return 0
      else
        echo "[FAILED] python3 binary not found, please install python3 (supported version 3.6, 3.7, 3.8)"
        exit 1
      fi
    else
      echo "[OK] python3 binary defined in \$PYTHON3_BIN as '$PYTHON3_BIN'"
      check_python3_bin
      return 0
    fi

}

#print_install_instructions_for_python3_bin
find_python3_bin
run_python
