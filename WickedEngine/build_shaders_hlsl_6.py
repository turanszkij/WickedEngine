import os
import xml.etree.ElementTree as ET

tree = ET.parse('WickedEngine_SHADERS.vcxproj')
root = tree.getroot()

namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"

file = open("build_PSSL.bat", "w")

for shader in root.iter(namespace + "FxCompile"):
    for shaderprofile in shader.iter(namespace + "ShaderType"):

        profile = shaderprofile.text
        name = shader.attrib["Include"]
        
        print profile + ":   " + name

        file.write("dxccompiler\dxc " + name + " -T ")
        
        if profile == "Vertex":
            file.write("vs")
        if profile == "Pixel":
            file.write("ps")
        if profile == "Geometry":
            file.write("gs")
        if profile == "Hull":
            file.write("hs")
        if profile == "Domain":
            file.write("ds")
        if profile == "Compute":
            file.write("cs")

        file.write("_6_0 ")

        file.write(" -flegacy-macro-expansion -Fo shaders_hlsl_6/" + os.path.splitext(name)[0] + ".cso \n")


    
file.close()
