#!/usr/bin/python3

import argparse
import glob
import subprocess
import time
import os
import shutil

parser = argparse.ArgumentParser()

parser.add_argument("-v", "--verbose", help="increase output verbosity", action="store_true")
parser.add_argument("--vulkan-sdk", type=str, help="Vulkan SDK path")
parser.add_argument("--build-path", type=str, help="build directory")
parser.add_argument("--build-flags-cc", type=str, help="additional flags to pass cc")
parser.add_argument("--build-flags-cxx", type=str, help="additional flags to pass c++")
parser.add_argument("--link-flags", type=str, help="additional flags to pass to c++ when linking")
parser.add_argument("--cc", type=str, help="specifies a c compiler to use")
parser.add_argument("--cxx", type=str, help="specifies a c++ compiler to use")
parser.add_argument("--linker", type=str, help="specifies a c++ linker to use")
parser.add_argument("-c", "--clean", help="cleans the contents of the build folder", action="store_true")

args = parser.parse_args()

if not args.cc:
    args.cc = "cc"

if not args.cxx:
    args.cxx = "c++"

if not args.linker:
    args.linker = args.cxx

if not args.build_flags_cc:
    args.build_flags_cc = ""

if not args.build_flags_cxx:
    args.build_flags_cxx = ""

if not args.link_flags:
    args.link_flags = ""

if not args.build_path:
    args.build_path = "build"

if not args.vulkan_sdk:
    # Find the Vulkan SDK directory. Assume it's in the home directory
    print("No Vulkan SDK path given")
    quit(-1)

# Setup cc/c++ flags
args.build_flags_cc = "" + args.build_flags_cc
args.build_flags_cxx = "-std=c++17 " + args.build_flags_cxx
#args.link_flags = "-L" + args.vulkan_sdk + "/lib -lvulkan -pthread" + args.link_flags

# Check to see if the build directory exists
if os.path.exists(args.build_path) and args.clean:
    shutil.rmtree(args.build_path)
    os.mkdir(args.build_path)

elif not os.path.exists(args.build_path):
    os.mkdir(args.build_path)

CFILES = glob.glob("WickedEngine/**/*.c", recursive=True)
CPPFILES = glob.glob("WickedEngine/**/*.cpp", recursive=True)
INCDIRS = [
    "WickedEngine",
    "WickedEngine/BULLET",
    "WickedEngine/fonts",
    "WickedEngine/LUA",
    "WickedEngine/Utility",
    args.vulkan_sdk + "/include",
    "/usr/lib/vulkan"
]

INC_CMD = []
for item in INCDIRS:
    INC_CMD += ["-I", item]

total_size = len(CFILES) + len(CPPFILES)

def mk_dir(path):
    paths = path.split("/")[:-1]

    dir_path = args.build_path
    for p in paths:
        dir_path += "/" + p
        if not os.path.exists(dir_path):
            os.mkdir(dir_path)

def compile_files(file_list, compiler, processed_files = 0):
    cmp_list = []
    for file in file_list:

        cmp_list += [args.build_path + "/" + file + ".o"]
        # See if the file has been updated since the last compile
        try:
            src_time = os.path.getmtime(file)
            cmp_time = os.path.getmtime(args.build_path + "/" + file + ".o")
            if (src_time == cmp_time):
                processed_files += 1
                continue
        except:
            pass
            
        cmd = [compiler, file, "-c", "-o", args.build_path + "/" + file + ".o"]
        cmd += INC_CMD

        # Include the file's directory
        inc = file.rfind("/")
        cmd += ["-I", file[:inc]]

        if compiler == args.cc:
            cmd += args.build_flags_cc.split()
        
        elif compiler == args.cxx:
            cmd += args.build_flags_cxx.split()

        if args.verbose:
            print(*cmd)
        
        print(
            "[", int(processed_files / total_size * 1000)/10.0, "% ]",
            "[", compiler, "]", file, "->", args.build_path + "/" + file + ".o"
        )
        
        mk_dir(file)

        subprocess.run(cmd)

        # Set the newly created file's last modified time as the same as it's source
        src_time = os.path.getmtime(file)

        try:
            os.utime(args.build_path + "/" + file + ".o", (src_time, src_time))
        except:
            quit()

        processed_files += 1
    
    return processed_files, cmp_list

p, cobj = compile_files(CFILES, args.cc)
_, cxxobj = compile_files(CPPFILES, args.cxx, p)

print("[ 100.0 % ] [ Archive ] WickedEngine ->", args.build_path + "/libWickedEngine.a")

cmd = ["ar", "rcs", args.build_path + "/libWickedEngine.a"]
cmd += cobj + cxxobj

if args.verbose:
    print(*cmd)

subprocess.run(cmd)
