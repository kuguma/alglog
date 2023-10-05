
from dataclasses import dataclass
from typing import Any
import sys
import inspect


@dataclass
class CmdItem:
    desc: str
    detail: str
    func: Any

class CommandRouter:
    def __init__(self):
        self.cmds = {}

    def add(self, func, desc="", detail="(no detail)"):
        self.cmds[func.__name__] = CmdItem(desc, detail, func)

    def route(self, name, *args):
        if not name in self.cmds.keys():
            print("[unknown command]")
        else:
            try:
                self.cmds[name].func(*args)
            except Exception as e:
                print( "[error]" )
                print( e )

    def print_detail(self, name):
        if not name in self.cmds.keys():
            print("[unknown command]")
        else:
            func_def_str = self.get_def_string(self.cmds[name].func)
            print(f"command [ {func_def_str} ] :\n{self.cmds[name].detail}")

    def commands(self):
        return ", ".join(self.cmds.keys())

    def help_str(self):
        s = ""
        for name, c in self.cmds.items():
            s += f"  {name:<15} {c.desc}\n"
        return s

    def get_def_string(self, func):
        source_code, _ = inspect.getsourcelines(func)
        return source_code[0].strip()[4:-1]

    def start(self):
        while True:
            print("> ", end="")
            cmd = input()
            cmd_splitted = cmd.split(" ")
            length = len(cmd_splitted)
            if length == 0:
                continue
            if length == 1 and cmd_splitted[0] == "exit":
                sys.exit(0)
            if cmd_splitted[0] == "help":
                if length == 1:
                    print(f"commands : \n{self.help_str()}")
                if length == 2:
                    self.print_detail(cmd_splitted[1])
            else:
                self.route(cmd_splitted[0], *cmd_splitted[1:])





    