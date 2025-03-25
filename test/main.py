from utility import *
from cmd_router import *

import platform

"""
usage : 

$ cd alglog
$ python test/main.py

"""

router = CommandRouter()

def build(config="Debug"):
    if config not in ["Debug","Release"]:
        print(f"invalid config = {config}")
        return
    shell(f"cmake -B build -DCMAKE_BUILD_TYPE={config} -DALGLOG_BUILD_TESTS=ON")
    shell(f"cmake --build build --config {config}")
    if platform.system() == "Windows":
        if config == "Debug":
            shell(f"{os.getcwd()}/build/test/Debug/AlgLogTest.exe")
        else:
            shell(f"{os.getcwd()}/build/test/Release/AlgLogTest.exe")
    else:
        shell(f"{os.getcwd()}/build/test/AlgLogTest")
router.add(build, "build {Debug | Release}")

def clean():
    rm_dir_if_exists("build")
router.add(clean)

def main():
    router.start()


main()