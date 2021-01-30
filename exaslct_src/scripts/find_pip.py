import subprocess
import sys
from typing import List

def print_ok(check_name:str, message:str=""):
    print(f"[OK]     {check_name}: {message}",file=sys.stderr)

def print_failed(check_name:str, message:str):
    print(f"[FAILED] {check_name}: {message}",file=sys.stderr)

def ensure_pip()->List[str]:
    check_name="ensure pip"
    pip_command_array=[sys.executable, '-m', 'pip']
    completed_process = subprocess.run(pip_command_array+['--version'],stderr=subprocess.STDOUT,stdout=subprocess.PIPE)
    if completed_process.returncode==0:
        pip_version=completed_process.stdout.decode("utf-8")
        print_ok(check_name, f"found '{pip_version}' for '{sys.executable}'")
    else:
        error_message=completed_process.stdout.decode("utf-8")
        print_failed(check_name, f"Could not find pip, got the following error:\n{error_message}")
        pip_install_command_array=[sys.executable, "-m", "ensurepip", "--user"]
        pip_install_command_str=" ".join(pip_install_command_array)
        answer=input(f"Do you want to install pip via '{pip_install_command_str}' to your user? yes/no: ")
        if answer == "yes":
            print(f"Installing pip via '{pip_install_command_str}'!")
            completed_process = subprocess.check_call(pip_install_command_array)
            return pip_command_array
        else:
            print(f"Please install pip manually for '{sys.executable}', or ask your adminestrator!")
            return None

pip_command_array=ensure_pip()
if pip_command_array is not None:
    exit(0)
else:
    exit(1)
