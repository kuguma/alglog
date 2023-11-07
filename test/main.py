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
    shell(f"cmake -B build -DCMAKE_BUILD_TYPE={config}")
    shell(f"cmake --build build --config {config}")
    if platform.system() == "Windows":
        if config == "Debug":
            shell(f"{os.getcwd()}/build/Debug/AlgLog.exe")
        else:
            shell(f"{os.getcwd()}/build/Release/AlgLog.exe")
    else:
        shell(f"{os.getcwd()}/build/AlgLog")
router.add(build, "build {Debug | Release}")

def clean():
    rm_dir_if_exists("build")
router.add(clean)

def main():
    router.start()


main()