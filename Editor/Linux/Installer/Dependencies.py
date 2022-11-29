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

def GetDistributionPackage(packageID: str):
    global distro

    match distro:
        case "Arch Linux":
            match packageID:
                case "TkInter":
                    packageID = "tk"
                    return packageID
        case "Ubuntu":
            match packageID:
                case "TkInter":
                    packageID = "python-tk"
                    return packageID


# Ask to install a Python dependency.
def RequestDependency(packageName: str):
    global distro

    loop: bool = True
    answer = input(
        f":: Would you like to install \033[0;33m{packageName}\033[0m? [y/n]"
    )

    if answer.lower() == "y":
        match distro:
            case "Arch Linux":
                subprocess.run(
                    f"sudo pacman -S --noconfirm {GetDistributionPackage(packageName)}",
                    shell=True,
                )
            case _:
                print(
                    f":: Could not detect your Linux distribution.\n:: You'll have to manually install {packageName}"
                )
                sys.exit()
    else:
        sys.exit(0)
