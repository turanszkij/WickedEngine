#!/usr/bin/python

import os
import time 

os.system('clear') # Clear the console to make things easier to read.

answer_installModuleDependencies = input("This installer has some Python dependencies, would you like to install them? [Yes/No]" + '\n')

if answer_installModuleDependencies == "Yes":
    print("Installing required modules.")
    os.system('pip install pillow ttkbootstrap distro')
elif answer_installModuleDependencies == "No": 
    print("Skipping module dependencies!")
    print("The installer might not run.")
    time.sleep(1)
elif answer_installModuleDependencies == "yes":
    print("Installing required modules.")
    os.system('pip install pillow ttkbootstrap distro')
elif answer_installModuleDependencies == "no": 
    print("Skipping module dependencies!")
    print("The installer might not run.")
    time.sleep(1)
else:
    print("Skipping module dependencies!")
    print("The installer might not run.")
    time.sleep(1)

import tkinter
from tkinter import filedialog
from PIL import Image, ImageTk
import ttkbootstrap as TkBootstrap 
from ttkbootstrap.constants import *
from ttkbootstrap.dialogs import * 	
import Dependencies

def SelectDirectory():
    print("Please select your Wicked Engine root directory.")
    installDirectory = tkinter.filedialog.askdirectory()
    
    # Making sure if this is actually a Wicked Engine root directory.
    if os.path.exists(installDirectory + '/WickedEngine.sln'):
        if os.path.exists(installDirectory + '/build/Editor/WickedEngineEditor'):
            print("Starting installation process!")
            
            # Binary installation
            os.system('sudo rm /usr/bin/wicked-engine')
            os.system('sudo cp ' + installDirectory + '/build/Editor/WickedEngineEditor /usr/bin/wicked-engine')
            os.system('sudo chmod +x /usr/bin/wicked-engine')
            # Desktop file installation
            os.system('sudo cp ' + installDirectory + '/Editor/Linux/Distribution/wicked-engine.desktop /usr/share/applications/wicked-engine.desktop')
            
            print("Finished installing!")
           
    else:
        print("It doesn't seem like you selected a Wicked Engine directory.")
        Messagebox.ok("It doesn't seem like you selected a Wicked Engine directory.", title = "Error", alert=True)       
            

if __name__ == "__main__":
    answer_installDependencies = input("Install package dependencies? [Yes/No]" + '\n')
        
    if answer_installDependencies == "Yes":
        print("Installing dependencies...")
        Dependencies.InstallPackages() 
    elif answer_installDependencies == "No":
        print("Skipping dependency installation!")
        time.sleep(1)
    elif answer_installDependencies == "yes":
        print("Installing dependencies...")
        Dependencies.InstallPackages() 
    elif answer_installDependencies == "no":
        print("Skipping dependency installation!")
        time.sleep(1)
    else:
        print("Skipping dependency installation!")
        time.sleep(1)
    
    #region Tk Interface variables.
    
    window = TkBootstrap.Window()
    window.title("Wicked Engine Installer")
    style = TkBootstrap.Style("flatly")
    
    # Window information is a float, we need an int, things break.
    screenWidth = int(window.winfo_screenwidth()/3) 
    screenHeight = int(window.winfo_screenheight()/3) 
    
    label_info = TkBootstrap.Label(text = "Please select the path to your Wicked Engine root folder." + '\n' + "Afterwards, the installation can begin.")
    button_selectPath = TkBootstrap.Button(text = "select path", command = SelectDirectory, bootstyle = 'success')
    
    #endregion
   
    os.system('clear') # Clear the console to make things pretty! 

    #region Window changes, widget packing/placing, etc. 
        
    window.geometry(str(screenWidth) + 'x' + str(screenHeight)) # Needs to be strings for some reason?
    
    button_selectPath.place(relx = 0.5, rely = 0.5, anchor = CENTER)
    label_info.place(relx = 0.5, rely = 0.25, anchor = CENTER)
    
    window.mainloop() # No changes should be applied after this.
    
    #endregion
    

    
     
