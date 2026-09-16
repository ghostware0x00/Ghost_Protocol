from pathlib import Path
import dispatcher
import colors

def main():
    print(f"{colors.Style.BRIGHT}{colors.Fore.CYAN}[*] Initiating ghost protocol")
    banner_path = Path(__file__).resolve().parent.parent / "banner" / "banner.txt"
    if banner_path.exists():
        with open(banner_path, "r") as file:
            print(file.read(),"\n")
    dispatcher.dispatch()


if __name__ == "__main__":
    main()