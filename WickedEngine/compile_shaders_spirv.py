import os
import xml.etree.ElementTree as ET
from subprocess import check_output

# The shader compiler scripts parses the Shaders_SOURCE project filters to determine which shaders to compile and how
#   Right now it depends on dxc.exe and dxcompiler.dll being available in the same directory as the script

tree = ET.parse('Shaders_SOURCE.vcxitems.filters')
root = tree.getroot()

outputdir = "spirv"

from pathlib import Path
Path("shaders/" + outputdir).mkdir(parents=True, exist_ok=True)


namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"
for shader in root.iter(namespace + "FxCompile"):
    for shaderprofile in shader.iter(namespace + "Filter"):

        profile = shaderprofile.text
        name = shader.attrib["Include"]
        name = name.replace("$(MSBuildThisFileDirectory)", "")

        cmd = "dxc " + name + " -T "
        
        if profile == "VS":
            cmd += "vs"
        if profile == "PS":
            cmd += "ps"
        if profile == "GS":
            cmd += "gs"
        if profile == "HS":
            cmd += "hs"
        if profile == "DS":
            cmd += "ds"
        if profile == "CS":
            cmd += "cs"
        if profile == "LIB":
            cmd += "lib"

        cmd += "_6_4 "

        cmd += "-D SPIRV -D "

        #cmd += "-D INLINE_RAYTRACING "
        
        if profile == "VS":
            cmd += "SPIRV_SHADERSTAGE=0 -fvk-invert-y"
        if profile == "HS":
            cmd += "SPIRV_SHADERSTAGE=1"
        if profile == "DS":
            cmd += "SPIRV_SHADERSTAGE=2 -fvk-invert-y"
        if profile == "GS":
            cmd += "SPIRV_SHADERSTAGE=3 -fvk-invert-y"
        if profile == "PS":
            cmd += "SPIRV_SHADERSTAGE=4"
        if profile == "CS" or profile == "LIB":
            cmd += "SPIRV_SHADERSTAGE=5"

        cmd += " -spirv -fvk-use-dx-layout -flegacy-macro-expansion -Fo " + "shaders/" + outputdir + "/" + os.path.splitext(name)[0] + ".cso "

        cmd += "-fspv-target-env=vulkan1.2 "

        print(cmd)

        try:
            print(check_output(cmd, shell=True).decode())
        except:
            print("DXC error")
        
        break


