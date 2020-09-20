import os
import xml.etree.ElementTree as ET
from subprocess import check_output

# The shader compiler scripts parses the Shaders_SOURCE project filters to determine which shaders to compile and how
#   Right now it depends on dxc.exe and dxcompiler.dll being available in the same directory as the script
#   dxil.dll will be used to sign the shader binaries if it is available in the same directory as the script

tree = ET.parse('Shaders_SOURCE.vcxitems.filters')
root = tree.getroot()

outputdir = "hlsl6"

from pathlib import Path
Path("shaders/" + outputdir).mkdir(parents=True, exist_ok=True)


namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"
for item in root.iter():
    try:
        name = item.attrib["Include"]
        filename, file_extension = os.path.splitext(name)
        if file_extension != ".hlsl":
            continue
        name = name.replace("$(MSBuildThisFileDirectory)", "")
        for shaderprofile in item.iter(namespace + "Filter"):
            profile = shaderprofile.text

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
            if profile == "MS":
                cmd += "ms"
            if profile == "AS":
                cmd += "as"

            cmd += "_6_5 "

            cmd += " -Fo " + "shaders/" + outputdir + "/" + os.path.splitext(name)[0] + ".cso "
            
            cmd += " -flegacy-macro-expansion "
            
            cmd += " -D HLSL6 "
            #cmd += " -D RAYTRACING_INLINE "
            #cmd += " -D RAYTRACING_GEOMETRYINDEX "
            
            print(cmd)
            
            try:
                print(check_output(cmd, shell=True).decode())
            except:
                print("DXC error")
            
            break
            
    except:
        continue


