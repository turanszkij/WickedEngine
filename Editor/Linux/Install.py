#!/usr/bin/python

import os

os.system('clear')

answer_installModuleDependencies = input("This installer has some Python dependencies, would you like to install them? [Yes/No]" + '\n')

if answer_installModuleDependencies == "Yes":
    print("Installing required modules.")
    os.system('pip install pillow ttkbootstrap distro')
elif answer_installModuleDependencies == "No": 
    print("Skipping module dependencies!" + '\n' + "The installer might not run.")
elif answer_installModuleDependencies == "Yes":
    print("Installing required modules.")
    os.system('pip install pillow ttkbootstrap distro')
elif answer_installModuleDependencies == "No": 
    print("Skipping module dependencies!" + '\n' + "The installer might not run.")
else:
    print("Skipping module dependencies!" + '\n' + "The installer might not run.")

import tkinter 
from PIL import Image, ImageTk
import ttkbootstrap as TkBootstrap 
from ttkbootstrap.constants import *
import Dependencies

def SelectDirectory():
    print("Please select your Wicked Engine root directory.")

if __name__ == "__main__":
    answer_installDependencies = input("Install package dependencies? [Yes/No]" + '\n')
        
    if answer_installDependencies == "Yes":
        print("Installing dependencies...")
        Dependencies.InstallPackages() 
    elif answer_installDependencies == "No":
        print("Skipping dependency installation!")
    elif answer_installDependencies == "yes":
        print("Installing dependencies...")
        Dependencies.InstallPackages() 
    elif answer_installDependencies == "no":
        print("Skipping dependency installation!")
    else:
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
    

    
     
