import dispatcher
import colors

def main():
    print(f"{colors.Style.BRIGHT}{colors.Fore.CYAN}[*] Initiating ghost protocol")
    with open("../banner/banner.txt", "r") as file:
        print(file.read(),"\n")
    dispatcher.dispatch()


if __name__ == "__main__":
    main()