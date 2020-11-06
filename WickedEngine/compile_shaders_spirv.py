import os
import threading
import xml.etree.ElementTree as ET
from subprocess import check_output

# The shader compiler scripts parses the Shaders_SOURCE project filters to determine which shaders to compile and how
#   Right now it depends on dxc.exe and dxcompiler.dll being available in the same directory as the script

tree = ET.parse('Shaders_SOURCE.vcxitems.filters')
root = tree.getroot()

from pathlib import Path
Path("shaders/spirv").mkdir(parents=True, exist_ok=True)

threads = []

def compile(cmd):
    try:
        print(cmd)
        result = check_output(cmd, shell=True).decode()
        if len(result) > 0:
            print(result)
    except:
        print("DXC error")

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
            
            cmd += " -spirv "
            cmd += " -fspv-target-env=vulkan1.2 "
            cmd += " -fvk-use-dx-layout "
            cmd += " -fvk-use-dx-position-w "
            cmd += " -flegacy-macro-expansion "
            
            if profile == "VS" or profile == "DS" or profile == "GS":
                cmd += " -fvk-invert-y "

            #cmd += " -fvk-b-shift 0 all "
            cmd += " -fvk-t-shift 1000 all "
            cmd += " -fvk-u-shift 2000 all "
            cmd += " -fvk-s-shift 3000 all "
            
            cmd += " -D SPIRV "

            output_name = os.path.splitext(name)[0] + ".cso "
            
            cmd += " -Fo " + "shaders/spirv/" + output_name
            t = threading.Thread(target=compile, args=(cmd,))
            threads.append(t)
            t.start()
            
            break
            
    except:
        continue
        

for t in threads:
    t.join()

