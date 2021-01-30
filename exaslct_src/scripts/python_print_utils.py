import sys

def print_ok(check_name:str, message:str=""):
    print(f"[OK]     {check_name}: {message}",file=sys.stderr)

def print_failed(check_name:str, message:str):
    print(f"[FAILED] {check_name}: {message}",file=sys.stderr)
