import unittest
from pathlib import Path
import subprocess as sp
import os
from tempfile import NamedTemporaryFile
from typing import List

script_dir = Path(__file__).resolve().parent
test_subjects_dir = script_dir.parent
print_outputs_for_success = False


def print_subprocess_output(cp):
    print(cp.stdout.decode("utf-8"))

def run_docker(args:List[str]):
    cp=sp.run(
        ["docker"]+args,
        check=True,stdout=sp.PIPE,stderr=sp.STDOUT)
    return cp

def build_docker_image(dockerfile:Path, image_name:str)->str:
    with NamedTemporaryFile("w") as tempfile_obj:
        with dockerfile.open("r") as dockerfile_obj:
            tempfile_obj.write(dockerfile_obj.read())
            tempfile_obj.write("\n")
            tempfile_obj.write("RUN mkdir /files_to_test\n")
            tempfile_obj.write("COPY * /files_to_test/\n")
        tempfile_obj.flush()
        try:    
            cp=run_docker([
                "build",
                "-t",image_name,
                "-f",tempfile_obj.name,
                str(test_subjects_dir)])
            if print_outputs_for_success:
                print_subprocess_output(cp)
        except sp.CalledProcessError as e:
            print_subprocess_output(e)
            raise e

def remove_docker_image(image_name:str)->str:
    try:    
        cp=run_docker(["rmi",image_name])
        if print_outputs_for_success:
            print_subprocess_output(cp)
    except sp.CalledProcessError as e:
        print_subprocess_output(e)
        print(e)

def run_docker_container(image_name:str, command:List[str]):
    cp=run_docker(["run", "--rm", image_name]+command)
    if print_outputs_for_success:
        print_subprocess_output(cp)
    return cp


class NoPython(unittest.TestCase):

    def __init__(self, methodName:str, os:str):
        super().__init__(methodName)
        dockerfile_name=f"Dockerfile.{os}_no_python"
        self.image_name=f"{self.__class__.__name__}".lower()
        self.dockerfile_path=Path(script_dir,dockerfile_name)

    def setUp(self):
        build_docker_image(self.dockerfile_path, self.image_name)
    
    def tearDown(self):
        remove_docker_image(self.image_name)
        pass


class UbuntuNoPython(NoPython):
    def __init__(self, methodName:str):
        super().__init__(methodName, "ubuntu")

    def test(self):
        try:
            run_docker_container(self.image_name, ["/files_to_test/run_find_python.sh"])
            self.fail("Did not throw CalledProcessError")
        except sp.CalledProcessError as e:
            self.assertIn("[FAILED] python binary: not found",e.stdout.decode("utf-8"))

if __name__ == '__main__':
    unittest.main()


