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


# Ask to install a Python dependency.
def RequestDependency(package: str):
    loop: bool = True
    answer = input(f":: Would you like to install \033[0;33m{package}\033[0m? [y/n]")

    if answer.lower() == "y":
        if GetDistribution() == "Arch Linux":
            subprocess.run("sudo pacman -S --noconfirm tk", shell=True)
    else:
        sys.exit(0)
