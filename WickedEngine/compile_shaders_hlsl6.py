import os
import xml.etree.ElementTree as ET
from subprocess import check_output

tree = ET.parse('Shaders_SOURCE.vcxitems')
root = tree.getroot()

## Hardcode visual studio namespace for now...
namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"

outputdir = "hlsl6"

from pathlib import Path
Path("shaders/" + outputdir).mkdir(parents=True, exist_ok=True)


## Then we parse the default shader project and generate build task for DXC compiler:
for shader in root.iter(namespace + "FxCompile"):
    for shaderprofile in shader.iter(namespace + "ShaderType"):

        profile = shaderprofile.text
        name = shader.attrib["Include"]
        name = name.replace("$(MSBuildThisFileDirectory)", "")
        
        print(profile + ":   " + name)

        cmd = "dxc " + name + " -T "
        
        if profile == "Vertex":
            cmd += "vs"
        if profile == "Pixel":
            cmd += "ps"
        if profile == "Geometry":
            cmd += "gs"
        if profile == "Hull":
            cmd += "hs"
        if profile == "Domain":
            cmd += "ds"
        if profile == "Compute":
            cmd += "cs"
        if profile == "Library":
            cmd += "lib"

        cmd += "_6_4 "

        cmd += "-D HLSL6 "

        #cmd += "-D INLINE_RAYTRACING "

        cmd += "-flegacy-macro-expansion -Fo " + "shaders/" + outputdir + "/" + os.path.splitext(name)[0] + ".cso "

        print(cmd)

        try:
            print(check_output(cmd, shell=True).decode())
        except:
            print("DXC error")
        
        break


