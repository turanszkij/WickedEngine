import os
import subprocess
from subprocess import Popen, PIPE
import sys
from tkinter import messagebox, filedialog

installerDirectory = os.path.dirname(os.path.realpath(__file__))
installerDirectorySelected: bool = False


def Install():
    global installerDirectory
    global installerDirectorySelected

    installCommands: str = [
        f"sudo cp {installerDirectory}/Distribution/wicked-engine.desktop /usr/share/applications/wicked-engine.desktop",
        f"sudo cp {installerDirectory}/Distribution/wicked-engine.sh /usr/bin/wicked-engine",
        f"sudo chmod +x {installerDirectory}/Distribution/wicked-engine.sh",
        f"sudo chmod +x /usr/bin/wicked-engine",
    ]

    if installerDirectorySelected:
        for installCommand in installCommands:
            subprocessTask = Popen(
                installCommand,
                shell=True,
                stderr=PIPE
            )


            if subprocessTask.wait() != 0:
                error = subprocessTask.communicate()
                print(error)

                errorMessage = messagebox.showerror(
                    title="Install Error", message=error
                )
                
                break

        if subprocessTask.returncode == 0:
            print(":: Finished installing!")
    else:
        directoryWarning = messagebox.showerror(
            title="Install Error", message="No Wicked Engine directory selected!"
        )


def SelectDirectory():
    global installerDirectorySelected

    installerDirectory = filedialog.askdirectory(
        initialdir="../../../", title="Select Wicked Folder"
    )

    installerDirectorySelected = True
