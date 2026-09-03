import colors

def display_sessionInfo(session_info, sessionCount):
    print(f"{colors.Style.BRIGHT}{colors.Fore.GREEN}[+]Total active sessions : {sessionCount}")
    if sessionCount > 0:
        print("_"*35)
        print(
            f"{'SESSION':<10}"
            f"{'TARGET IP':<15}"
            f"{'PORT':<10}"
        )
        print("_"*35)
        for session in session_info:
            print(
                f"{session['agent_sid']:<10}"
                f"{session['agent_ip']:<15}"
                f"{session['agent_port']:<10}"
            )
        print("_"*35)
    else:
        print(f"{colors.Style.RED}[!] no active sessions present")
    print()
