import os
import distro

def InstallPackages():
    if distro.name() == "Fedora Linux":
        # Dependency installation    
        os.system('sudo dnf install SDL2 SDL2-devel vulkan vulkan-devel vulkan-headers gcc-c++')  
