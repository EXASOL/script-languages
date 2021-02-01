#!/bin/bash

set -u

find_docker_script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

source "$find_docker_script_dir"/print_utils.sh

function get_min_supported_major_docker_server_version () {
  echo "17"
}

function get_min_supported_minor_docker_server_version () {
  echo "05"
}

print_install_instructions_for_docker () {
    (
        echo "Please follow the installation instraction on https://docs.docker.com/get-docker/"
        echo "or for a quick installation for non production linux machines the instruction on https://github.com/docker/docker-install"
    ) >&2
}

get_docker_server_version () {
    local docker_bin="$1"
    "$docker_bin" info | grep  "Server Version: " | cut -f2 -d ":" | tr -d '[:space:]'
}

check_docker_server_version () {
    local docker_bin="$1"
    local docker_server_version=$(get_docker_server_version "$docker_bin")
    local docker_major_server_version=$(echo "$docker_server_version" | cut -f1 -d ".")
    local docker_minor_server_version=$(echo "$docker_server_version" | cut -f2 -d ".")
    local min_supported_major_docker_server_version=$(get_min_supported_major_docker_server_version)
    local min_supported_minor_docker_server_version=$(get_min_supported_minor_docker_server_version)
    local check_name="docker server version"
    local version_supported=false 
    if [[ "$docker_major_server_version" -ge "$min_supported_major_docker_server_version" ]]
    then
        if [[ "$docker_major_server_version" -gt "$min_supported_major_docker_server_version" ]]
        then
            version_supported=true
        else
            if [[ "$docker_minor_server_version" -ge "$min_supported_minor_docker_server_version" ]]
            then
                version_supported=true
            fi
        fi
    fi
    if [[ "$version_supported" == "true"  ]]
    then
        print_ok "$check_name" "docker '$docker_server_version' supported"
        return 0
    else
      print_failed "$check_name" "'$docker_server_version' not supported, please install docker (minimum supported version $min_supported_major_docker_server_version.$min_supported_minor_docker_server_version)"
      print_install_instructions_for_docker
      return 1
    fi
}

check_docker_bin_available () {
    local docker_bin="$1"
    local check_name="docker binary is executable"
    local docker_bin_test_run=$("$docker_bin" --version 2>&1)
    if [ "$?" -eq 0 ]
    then
        print_ok "$check_name" ""
        return 0
    else
        print_failed "$check_name" "'$docker_bin' can't be executed, got following error\n$docker_bin_test_run"
        print_install_instructions_for_docker
        return 1
    fi
}

check_docker_pull () {
    local docker_bin="$1"
    local check_name="docker pull"
    local docker_pull_test_run=$("$docker_bin" pull ubuntu:18.04 2>&1)
    if [ "$?" -eq 0 ]
    then
        print_ok "$check_name" ""
        return 0
    else
        print_failed "$check_name" "Pulling images doesn't work for '$docker_bin' , got following error\n$docker_pull_test_run"
        return 1
    fi
}

check_docker_internet () {
  local docker_bin="$1"
  local check_name="docker container internet connectivity"
  local docker_internet_test_run=$("$docker_bin" run --rm ubuntu:18.04 bash -c ": >/dev/tcp/1.1.1.1/53")
  if [ "$?" -eq 0 ]
  then
      print_ok "$check_name" ""
      return 0
  else
      print_failed "$check_name" "Docker container can't access the internet for '$docker_bin' , got following error\n$docker_internet_test_run"
      return 1
  fi
}

check_docker_dns () {
    local docker_bin="$1"
    local check_name="docker container dns"
    local docker_dns_test_run=$("$docker_bin" run --rm ubuntu:18.04 bash -c ": >/dev/tcp/ubuntu.com/80")
    if [ "$?" -eq 0 ]
    then
        print_ok "$check_name" ""
        return 0
    else
        print_failed "$check_name" "Docker container can't resolve domain names for '$docker_bin' , got following error\n$docker_dns_test_run"
        return 1
    fi
}

check_docker () {
    local docker_bin="$1"
    if ! check_docker_bin_available "$docker_bin"
    then
        return 1
    fi
    if ! check_docker_server_version "$docker_bin"
    then
        return 1
    fi
    if ! check_docker_pull "$docker_bin"
    then
        return 1
    fi
    if ! check_docker_internet "$docker_bin"
    then
        return 1
    fi
    if ! check_docker_dns "$docker_bin"
    then
        return 1
    fi
    return 0
}


find_docker_bin () {
    local check_name="docker binary"
    if [ ! "${EXASLCT_DOCKER_BIN+x}" = "x" ]
    then
      docker_bin=$(command -v docker)
      if [ -n "$docker_bin" ]
      then
        print_ok "$check_name" "found at '$docker_bin'"
      else
        local min_supported_major_docker_server_version=$(get_min_supported_major_docker_server_version)
        local min_supported_minor_docker_server_version=$(get_min_supported_minor_docker_server_version)
        print_failed "$check_name" "not found, please install docker (minimum supported version $min_supported_major_docker_server_version.$min_supported_minor_docker_server_version)"
        print_install_instructions_for_docker
        return 1
      fi
    else
      local docker_bin=${EXASLCT_DOCKER_BIN}
      print_ok "$check_name" "defined in \$EXASLCT_DOCKER_BIN as '$docker_bin'"
    fi
    if check_docker "$docker_bin"
    then
      echo "$docker_bin"
      return 0
    else
      return 1
    fi
}

find_docker_bin
