import sys
import os
import subprocess
import cli
import connection

def help_usage():
    help_commands = [
        "help",
        "sessions",
        "use session", # using this we login to the target's shell and then execute commands 
        "back",
        "clear",
        #"execute <command>",
        "exit"
    ]
    print(f"Available Commands : ")
    print(f"\n".join(help_commands))


def sessions_usage():
    print(f"[*] Listing sessions")
    connection.connect("sessions")



def clear_usage():
    subprocess.run("clear", shell=True)


def back_usage():
    pass



def execute_cmd_usage():
    pass



def exit_usage():
    print(f"[-]Exiting Operator Console")
    sys.exit() # returns statsu code 0 and exits program


# key value pairing is done like key <-> function_name
# we can trigger this function using the key anytime we like
command_dispatcher = {
    "help": help_usage,
    "sessions": sessions_usage,
    "back": back_usage,
    "execute": execute_cmd_usage,
    "clear": clear_usage,
    "exit": exit_usage
}

def dispatch():
    print(f"[+] %%%%%%% starting ghost protocol %%%%%%%")
    while True:
        try:
            command = cli.command_input_prompt()
            if command in command_dispatcher:
                command_dispatcher[command]() # using the command key to call the associated function of the key")
            else:
                print(f"gho$t shell usage :- ")
                command_dispatcher["help"]()
        except KeyboardInterrupt:
            print(f"\n[-]exiting ghost protocol console")
            os._exit(0)

        