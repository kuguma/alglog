import subprocess
import platform
import os, shutil
import time

# -------------------------------------------------------------------------------------------------

def shell(cmd, **kwargs):
    print(f'[ Shell ] {cmd}')
    try:
        # check=Trueの場合を考慮
        sp = subprocess.run(
            cmd, shell=True,
            check=True,
            # stdin=subprocess.DEVNULL,
            # stdout=subprocess.PIPE,
            # stderr=subprocess.STDOUT,
            **kwargs)
    except Exception as e:
        print(str(e))
        raise e
    return sp

# wsl上でコマンドを実行する。(Windowsのみ)
def wsl(cmd):
    shell(f'bash --login -i -c "{cmd}"')
    # shell(f'wsl -e "{cmd}"')

# ターミナルの画面をクリアする
def clear_screen():
    pf = platform.system()
    if pf == "Windows":
        subprocess.run("cls", shell=True)
    else:
        subprocess.run("clear", shell=True)

def rmtree_onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.

    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.

    If the error is for another reason it re-raises the error.

    Usage : ``shutil.rmtree(path, onerror=onerror)``
    """
    import stat
    # Is the error an access error?
    if not os.access(path, os.W_OK):
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        raise
# ディレクトリが存在していた場合に削除する
def rm_dir_if_exists(dir):
    if os.path.isdir(dir):
        try:
            shutil.rmtree(dir, onerror=rmtree_onerror)
        except Exception as e:
            print(str(e))
            raise e
        print(f"Removed directory: {dir}")

# 空のディレクトリを作成する。既に存在していた場合は削除する。
def remake_dir(dir):
    rm_dir_if_exists(dir)
    os.makedirs(dir)

# ファイルコピーを行う。ファイルが存在していた場合は上書きする。必要なディレクトリが存在しない場合は作成する。
def force_copyfile(src, dst):
    dst_dir = os.path.dirname(dst)
    if dst_dir != "":
        os.makedirs( dst_dir, exist_ok=True )
    shutil.copyfile(src, dst)

class ChangeDir:
    """与えられたパスに移動して、処理後に元のパスに戻る"""

    def __init__(self, fpath):
        self.fpath = fpath
        print(self.fpath)

    def __enter__(self):
        self.curdir = os.getcwd()
        os.chdir(self.fpath)

    def __exit__(self, exc_type, exc_value, traceback):
        os.chdir(self.curdir)

class TimeProc:
    """時間計測をして表示する"""

    def __init__(self, name):
        self.name = name

    def __enter__(self):
        self.start = time.time()
        print(f"[{self.name}] start.")

    def __exit__(self, exc_type, exc_value, traceback):
        elapsed = time.time() - self.start
        print(f"[{self.name}] finished in {elapsed} sec.")