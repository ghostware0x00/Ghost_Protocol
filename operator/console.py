import colors

def display_sessionInfo(sessionId_List, sessionCount):
    print(f"{colors.Style.BRIGHT}{colors.Fore.GREEN}[+]Total active sessions : {sessionCount}")
    if sessionCount > 0:
        print(f"-"*30)
        print(f"{'ID':<10}{'IP':<20}")
        print(f"-"*30)
        for sessions in sessionId_List:
            print(f"{colors.Fore.CYAN}{sessions:<10}")
    else:
        print(f"{colors.Fore.RED}[!] no active sessions available")
    print()
