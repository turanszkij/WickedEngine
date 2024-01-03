#!/usr/bin/env python3

import pywickedengine

dump_to_header = False

def main():
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

if __name__ == "__main__":
    main()
