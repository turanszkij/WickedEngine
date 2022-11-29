import subprocess

from tkinter import messagebox
from tkinter import filedialog

wickedRootDirectory = "WICKED"
wickedRootDirectorySelected = bool()


def Install():
    global wickedRootDirectory
    global wickedRootDirectorySelected

    if wickedRootDirectorySelected:
        subprocess.run(
            f"""sudo cp {wickedRootDirectory}/Editor/Linux/Installer/Distribution/wicked-engine.desktop /usr/share/applications/wicked-engine.desktop""",
            shell=True,
        )
        subprocess.run(
            f"sudo cp {wickedRootDirectory}/Editor/Linux/Installer/Distribution/wicked-engine.sh /usr/bin/wicked-engine",
            shell=True,
        )
        subprocess.run(
            f"sudo chmod +x {wickedRootDirectory}/Editor/Linux/Installer/Distribution/wicked-engine.sh",
            shell=True,
        )
        subprocess.run(f"sudo chmod +x /usr/bin/wicked-engine", shell=True)

        print(":: Finished installing!")
    else:
        directoryWarning = Messagebox.ok(
            "No Wicked Engine directory selected!", title="Install Error"
        )


def SelectDirectory():
    global wickedRootDirectorySelected

    wickedRootDirectory = filedialog.askdirectory(
        initialdir="../../../", title="Select Wicked Folder"
    )

    wickedRootDirectorySelected = True
