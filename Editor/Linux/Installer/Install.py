#!/usr/bin/python

#region Modules

import os

import tkinter as Tk
from tkinter import filedialog
from PIL import Image, ImageTk

import ttkbootstrap as TkBootstrap 
from ttkbootstrap.constants import *
from ttkbootstrap.dialogs import *

#endregion

#region Variables

wickedRootDirectory = "$WICKED"
wickedRootDirectorySelected = False

#endregion

#region Functions

def Install():
    if(wickedRootDirectorySelected):    
        os.system("sudo cp {wickedRootDirectory}/Editor/Linux/Installer/Distribution/usr/share/applications/wicked-engine.desktop /usr/share/applications/wicked-engine.desktop")
        os.system("sudo cp {wickedRootDirectory}/Distribution/usr/bin/wicked-engine /usr/bin/wicked-engine")
        
        # The two following `if` statements are for package maintainers or distributors to edit and use!
        # Please @MolassesLover on the Discord or create a GitHub issue if these statements break.

        #if(checkButton_denoiser.offvalue):
        #    pass
        #else:
        #    pass
        
        #if(checkButton_settings.offvalue):
        #    pass
        #else:
        #    pass

    else:
        directoryWarning = Messagebox.ok("No Wicked Engine directory selected!", title = "Install Error")

def SelectDirectory():
    wickedRootDirectory = filedialog.askdirectory(initialdir = "../../../", title = "Select Wicked Folder")
    wickedRootDirectorySelected = True 

#endregion

if __name__ == "__main__":
    #region Tk Interface variables
    
    window = TkBootstrap.Window()
    window.title("Wicked Engine Installer")
    window.resizable(False, False)
    style = TkBootstrap.Style("darkly")

    screenWidth = int(window.winfo_screenwidth()/3)
    screenHeight = int(window.winfo_screenheight()/5)

    #endregion

    #region Tk Interface widgets
    
    button_install = TkBootstrap.Button(text = "start install", bootstyle = 'success', command = Install)
    button_path = TkBootstrap.Button(text = "select source folder", bootstyle = 'default', command = SelectDirectory)
    checkButton_denoiser = TkBootstrap.Checkbutton(text = "add denoising", bootstyle = 'outline-toolbutton')
    checkButton_settings = TkBootstrap.Checkbutton(text = "copy settings", bootstyle = 'outline-toolbutton')
    entry_path = TkBootstrap.Entry(text = "root directory", bootstyle = 'default')
    label_info = TkBootstrap.Label(text = "Wicked Engine folder:")

    #endregion

    #region Window changes, widget packing/placing, etc.

    window.geometry(str(screenWidth) + 'x' + str(screenHeight)) # Needs to be strings for some reason?

    button_install.place(relx = 0.5, rely = 0.75, anchor = CENTER)
    button_path.place(relx = 0.5, rely = 0.5, anchor = CENTER)

    # These two lines are up to the package maintainer to use or not!
    #checkButton_denoiser.place(relx = 0.25, rely = 0.325, anchor = CENTER) 
    #checkButton_settings.place(relx = 0.25, rely = 0.650, anchor = CENTER)

    window.mainloop() # No changes should be applied after this.

    #endregion