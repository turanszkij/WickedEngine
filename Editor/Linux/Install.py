#!/usr/bin/python

import os
import distro
import tkinter
from PIL import Image, ImageTk
import ttkbootstrap as TkBootstrap 
from ttkbootstrap.constants import *

def SelectDirectory():
    print("Please select your Wicked Engine root directory.")

if __name__ == "__main__":
    answer_installDependencies = input("Install dependencies?" + '\n')
        
    if answer_installDependencies == "Yes":
        if distro.name() == "Fedora Linux":
            # Dependency installation
            print("Installing dependencies...")
            os.system('sudo dnf install SDL2 SDL2-devel vulkan vulkan-devel vulkan-headers gcc-c++')   
    elif answer_installDependencies == "No":
        print("Skipping dependency installation!")
    
    #region Tk Interface variables.
    
    window = TkBootstrap.Window()
    window.title("Wicked Engine Installer")
    
    style = TkBootstrap.Style("darkly")
    
    # Window information is a float, we need an int, things break.
    screenWidth = int(window.winfo_screenwidth()/3) 
    screenHeight = int(window.winfo_screenheight()/3)
    
    button_selectPath = tkinter.Button(text = "Select path", command = SelectDirectory)
    
    #endregion
   
    os.system('clear') # Clear the console to make things pretty! 

    #region Window changes, widget packing/placing, etc. 
        
    window.geometry(str(screenWidth) + 'x' + str(screenHeight)) # Needs to be strings for some reason?
    
    button_selectPath.place(relx = 0.5, rely = 0.75, anchor = CENTER)
    
    window.mainloop() # No changes should be applied after this.
    
    #endregion
    

    
     
