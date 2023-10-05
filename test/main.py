from utility import *
from cmd_router import *


router = CommandRouter()

def build():
    shell("cmake -B build")
    shell("cmake --build build")
    shell(f"{os.getcwd()}/build/Debug/AlgLog.exe")
router.add(build)

def main():
    router.start()


main()