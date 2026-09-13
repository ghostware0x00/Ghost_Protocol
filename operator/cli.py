import colors

def command_input_prompt(current_session):
    if current_session is None:
        prompt = "ghost$> "
    else:
        prompt = f"ghost [{current_session}]$> "
    command = input(
        f"{colors.Style.BRIGHT}"
        f"{colors.Fore.GREEN}"
        f"{prompt}"
        f"{colors.Style.RESET_ALL}"
    )
    return command


def shellPrompt():
    prompt = "agent$> "
    command = input(
        f"{colors.Style.BRIGHT}"
        f"{colors.Fore.MAGENTA}"
        f"{prompt}"
        f"{colors.Style.RESET_ALL}"
    )
    return command