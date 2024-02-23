#!/usr/bin/env python3

import os
import sys
importpath=os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(importpath, "wicked_blender_exporter"))
import pywickedengine

dump_to_header = False

def main():
    pywickedengine.init()

    filename = "/tmp/test.wiscene"
    ar = pywickedengine.Archive() if dump_to_header else pywickedengine.Archive(filename, False)
    if not ar.IsOpen():
        print("ERROR, ARCHIVE DID NOT OPEN")
        return

    scene = pywickedengine.Scene()
    scene.Serialize(ar)

    if dump_to_header:
        ar.SaveHeaderFile("/tmp/test.h","test")# wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(filename)));

    ar.Close()

    pywickedengine.deinit()

if __name__ == "__main__":
    main()
