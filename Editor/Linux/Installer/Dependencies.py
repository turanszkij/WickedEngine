import subprocess
import sys

# Global variable for all supported Linux distributions
distroList: str = ["Arch Linux"]

# Get the Linux distribution name from `/etc/os-release`.
def GetDistribution():
    distribution: str = "None"

    with open("/etc/os-release", "r") as releaseInformation:
        releaseInformationString: str = str(releaseInformation.read())

        for distro in distroList:
            if f'NAME="{distro}"' in releaseInformationString:
                distribution = distro
                print(f"Detected {distribution}")
                return distribution

distro = GetDistribution()

def InstallDistributionPackage(packageID: str):
    global distro

    match distro:
        case "Arch Linux":
            match packageID:
                case "TkInter":
                    command = "sudo pacman -S --noconfirm tk"
        case "Ubuntu":
            match packageID:
                case "TkInter":
                    command = "sudo apt-get install python-tk" 
        case _:
                print(
                    f":: Could not detect your Linux distribution.\n:: You'll have to manually install {packageName}"
                )
                sys.exit()
    subprocess.run(command, shell=True)


# Ask to install a Python dependency.
def RequestDependency(packageName: str):
    global distro

    loop: bool = True
    answer = input(
        f":: Would you like to install \033[0;33m{packageName}\033[0m? [y/n]"
    )

    if answer.lower() == "y":
        InstallDistributionPackage(packageName)
    else:
        sys.exit(0)
