#!/bin/python3 

# TODO I should rewrite this in C++ and maybe run it at runtime so I can bake as many features as I want in at runtime
# THEN I'd be able to dispatch this translation at runtime during a web build instead of running this script at build time or something

def file_to_string(path : str) -> str:
    s = ""
    with open(path, "r") as f:
        s = f.read()
    return s

def glsl_330_to_100(code : str, is_fragment : bool) -> str:
    new_code = ""
    is_precision_needed = True
    fragment_out_var = ""
    for line in code.split('\n'):
        if len(line) == 0 or line.startswith("//"):
            continue
        elif line.startswith("#version"):
            new_code += "#version 100"
        elif is_precision_needed:
            is_precision_needed = False 
            new_code += "precision mediump float;"
        elif line.startswith("in "):
            new_code += line.replace("in ", "varying ")
        elif is_fragment and line.startswith("out "):
            # pattern should always be "out vec4 variableName;"
            tokens = line.split(" ")
            fragment_out_var = tokens[2].replace(";", "").strip()
            # print("parsed fragment_out_var as ", fragment_out_var)
            continue
        elif "texture(" in line:
            new_code += line.replace("texture(", "texture2D(")
        else:
            new_code += line
        
        new_code += "\n"

    new_code = new_code.replace(fragment_out_var, "gl_FragColor")

    return new_code

if __name__ == "__main__":
    import sys 

    if len(sys.argv) > 1:
        path = sys.argv[1]
        code = file_to_string(path)
        is_fragment = True # TODO hard-coded
        code = glsl_330_to_100(code, is_fragment)
        print(code)
