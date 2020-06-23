import os
import xml.etree.ElementTree as ET

tree = ET.parse('WickedEngine/Shaders_SOURCE.vcxitems')
root = tree.getroot()

## Hardcode visual studio namespace for now...
namespace = "{http://schemas.microsoft.com/developer/msbuild/2003}"

file = open("build_SPIRV.bat", "w")

outputdir = "spirv"

## First, set error log output:
file.write("2>build_SPIRV_errors.log cls \n")

## Then, ensure that we have the output directory:
file.write("mkdir WickedEngine\shaders \n")
file.write("cd WickedEngine\shaders \n")
file.write("mkdir " + outputdir + "\n")
file.write("cd .. \n")

## Then we parse the default shader project and generate build task for an other shader compiler:
for shader in root.iter(namespace + "FxCompile"):
    for shaderprofile in shader.iter(namespace + "ShaderType"):

        profile = shaderprofile.text
        name = shader.attrib["Include"]
        name = name.replace("$(MSBuildThisFileDirectory)", "")
        
        print(profile + ":   " + name)

        file.write("..\shadercompilers\dxc " + name + " -T ")
        
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
        if profile == "Library":
            file.write("lib")

        file.write("_6_4 ")

        file.write("-D SHADERCOMPILER_SPIRV -D ");
        
        if profile == "Vertex":
            file.write("SPIRV_SHADERSTAGE=0 -fvk-invert-y")
        if profile == "Hull":
            file.write("SPIRV_SHADERSTAGE=1")
        if profile == "Domain":
            file.write("SPIRV_SHADERSTAGE=2 -fvk-invert-y")
        if profile == "Geometry":
            file.write("SPIRV_SHADERSTAGE=3 -fvk-invert-y")
        if profile == "Pixel":
            file.write("SPIRV_SHADERSTAGE=4")
        if profile == "Compute" or profile == "Library":
            file.write("SPIRV_SHADERSTAGE=5")

        file.write(" -spirv -fvk-use-dx-layout -flegacy-macro-expansion -Fo " + "shaders/" + outputdir + "/" + os.path.splitext(name)[0] + ".cso ")

        file.write("-fspv-target-env=vulkan1.2 ")
        
        ## Append to error log:
        file.write(" 2>>../build_SPIRV_errors.log \n")
        break


file.write("cd .. \n")
    
file.close()
