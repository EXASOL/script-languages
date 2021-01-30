from find_pip import ensure_pip
import sys
import subprocess
from python_print_utils import print_ok, print_failed
from typing import List

def ensure_pipenv(pip_command_array:List[str])->List[str]:
    check_name="ensure pipenv"
    pipenv_command_array=[sys.executable, '-m', 'pipenv']
    completed_process = subprocess.run(pipenv_command_array+['--version'],stderr=subprocess.STDOUT,stdout=subprocess.PIPE)
    if completed_process.returncode==0:
        pipenv_version=completed_process.stdout.decode("utf-8").strip()
        print_ok(check_name, f"found '{pipenv_version}' for '{sys.executable}'")
        return pipenv_command_array
    else:
        error_message=completed_process.stdout.decode("utf-8").strip()
        print_failed(check_name, f"Could not find pipenv, got the following error:\n{error_message}")
        pipenv_install_command_array=pip_command_array+["install", "--user", "pipenv"]
        pipenv_install_command_str=" ".join(pipenv_install_command_array)
        answer=input(f"Do you want to install pipenv via '{pipenv_install_command_str}' to your user? yes/no: ")
        if answer == "yes":
            print(f"Installing pipenv via '{pipenv_install_command_str}'!")
            completed_process = subprocess.run(pipenv_install_command_array)
            if completed_process.returncode == 0:
                print_ok(check_name, f"installed 'pipenv for '{sys.executable}'")
                return pipenv_command_array
            else:
                print_failed(check_name, "pipenv installation failed, please install pipenv manually or '{sys.executable}', or ask your adminestrator!")
                return None
        else:
            print(f"Please install pipenv manually for '{sys.executable}', or ask your adminestrator!")
            return None

pip_command_array=ensure_pip()
if pip_command_array is not None:
    ensure_pipenv(pip_command_array)
else:
    exit(1)
