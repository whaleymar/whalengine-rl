#!/usr/bin/env python3

import os 

BUILD_NAMES = ["engine", "engined"]
EXTENSIONS = [".exe", ".data", ".html", ".js", ".wasm", ".worker.js"]

for name in BUILD_NAMES:
    for ext in EXTENSIONS:
        filename = f"{name}{ext}"
        if (os.path.exists(filename)):
            os.remove(filename)
            print("removed ", filename)
