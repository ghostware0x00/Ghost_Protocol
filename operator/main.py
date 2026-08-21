import dispatcher

def main():
    print(f"[*] Initiating ghost protocol")
    with open("../banner/banner.txt", "r") as file:
        print(file.read(),"\n")
    dispatcher.dispatch()


if __name__ == "__main__":
    main()