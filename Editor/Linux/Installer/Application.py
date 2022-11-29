import Dependencies

try:
    from tkinter import *
except:
    print(":: \033[0;31mError\033[0m: Unable to find Tk Interface!")
    Dependencies.RequestDependency("TkInter")

import Installer


class InstallerApplication(Tk):
    def __init__(self):
        super().__init__()

        self.screenWidth = int(self.winfo_screenwidth() / 3)
        self.screenHeight = int(self.winfo_screenheight() / 5)

        self.title("Wicked Engine Installer")
        self.resizable(False, False)

        self.button_install = Button(text="start install", command=Installer.Install)
        self.button_path = Button(
            text="select source folder", command=Installer.SelectDirectory
        )
        self.checkButton_denoiser = Checkbutton(text="add denoising")
        self.checkButton_settings = Checkbutton(text="copy settings")
        self.entry_path = Entry(text="root directory")
        self.label_info = Label(text="Wicked Engine folder:")

        self.geometry(
            f"{self.screenWidth}x{self.screenHeight}"
        )  # Needs to be a string for some reason?

        self.button_install.place(relx=0.5, rely=0.75, anchor=CENTER)
        self.button_path.place(relx=0.5, rely=0.5, anchor=CENTER)

    def on_closing(self, event=0):
        self.destroy()
