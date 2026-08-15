import sys

def help_usage():
    help =(
        "help"
        "sessions"
        "use <session_id>"
        "back"
        "execute <command>"
        "exit"
    )
    print(f"Available Commands : ")
    print(f"\n".join(help))


def sessions_usage():
    pass



def back_usage():
    pass



def execute_cmd_usage():
    pass



def exit_usage():
    sys.exit() # returns statsu code 0 and exits program


# key value pairing is done like key <-> function_name
# we can trigger this function using the key anytime we like
command_dispatcher = {
    "help": help_usage,
    "sessions": sessions_usage,
    "back": back_usage,
    "execute": execute_cmd_usage,
    "exit": exit_usage
}
