import os 

errorFound = False;

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
        errorFound = True

walk("src/")
if (not errorFound):
    print("no issues found")
