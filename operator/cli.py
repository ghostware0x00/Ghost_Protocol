import colors

def command_input_prompt():
    command = input(f"{colors.Style.BRIGHT}{colors.Fore.GREEN}ghost$> {colors.Style.RESET_ALL}")
    return command