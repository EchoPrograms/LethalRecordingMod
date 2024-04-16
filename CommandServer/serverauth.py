import sys
import random


random.seed()


if __name__ != "__main__":
    exit(1)


chosenOptions = []
validOptions = "raR"
lastOption = "\0"
targetedUserR = ""
targetedUsera = ""
validKeyChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"


for arg in sys.argv:
    if lastOption == '\0' or lastOption == 'r':
        if arg[0] == "-":
            for opt in arg[1:]:
                    if opt not in chosenOptions and opt in validOptions:
                        chosenOptions.append(opt)
                        lastOption = opt
                    else:
                        print("Option", opt, "is repeated or invalid!")
                        exit(2)
    elif lastOption == 'R':
        targetedUserR = arg
    elif lastOption == 'a':
        targetedUsera = arg


if 'r' in chosenOptions:
    users = []
    authFile = open("Auth/users", "r")
    users = authFile.readlines()
    authFile.close()
    for user in users:
        keyFile = open("Auth/{}Key".format(user), "w")
        for i in range(0, 10):
            keyFile.write(validKeyChars[random.randrange(len(validKeyChars))])
        keyFile.close()


if 'R' in chosenOptions:
    keyFile = open("Auth/{}Key".format(targetedUserR), "w")
    for i in range(0, 10):
        keyFile.write(validKeyChars[random.randrange(len(validKeyChars))])
    keyFile.close()


if 'a' in chosenOptions:
    authFile = open("Auth/users", "a")
    authFile.write(targetedUsera)
    authFile.write("\n")
    authFile.close()
    keyFile = open("Auth/{}Key".format(targetedUsera), "w")
    for i in range(0, 10):
        keyFile.write(validKeyChars[random.randrange(len(validKeyChars))])
    keyFile.close()
    IPFile = open("Auth/{}IP".format(targetedUsera), "w")
    IPFile.write("Unknown")
    
