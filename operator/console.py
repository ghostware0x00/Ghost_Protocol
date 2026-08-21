def display_sessionInfo(sessionId_List, sessionCount):
    print(f"[+]Total active sessions : {sessionCount}")
    if sessionCount > 0:
        print(f"-"*30)
        print(f"{'ID':<10}{'IP':<20}")
        print(f"-"*30)
        for sessions in sessionId_List:
            print(f"{sessions:<10}")
    else:
        print(f"[-] no active sessions available")
    print()
