import os 

def walk(path):
    for f in os.listdir(path):
        fullpath = os.path.join(path, f)
        if os.path.isdir(fullpath):
            walk(fullpath)
        else:
            if fullpath.endswith(".h"):
                checkheader(fullpath)

def checkheader(path):
    with open(path, "r") as f:
        s = f.read()
    if not s.startswith("#pragma once"):
        print(path + " missing header guard")

walk("src/")
