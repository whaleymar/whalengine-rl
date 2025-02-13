#!/bin/python3 

"""
Prototype code for a custom shader compiler which converts Combined Shader Code (CSC) into GLSL vertex and fragment shaders

CSC is basically a GLSL 3.3 shader with a few extensions. It is inspired by Godot's shader language.

Here are the extensions which are the same as Godot's shader language:
1) The vertex and fragment shader can be defined in a single shader. The vertex `main` function is defined using a function with the signature `void vertex()` and the fragment shader with `void fragment()`. There should not be any `void main()` functions.
2) Variables which need to be passed from the vertex shader to the fragment shader should be declared with the `varying` keyword (e.g. `varying float scale;`)
3) constants, uniforms, and functions are automatically shared between vertex and fragment shaders. The compiler checks for usage, so the compiled GLSL will only include what's actually used in each shader.
4) Uniforms can have default values (stored in the compiler metadata for engine use).
5) Uniforms can have range hints (compiler metadata).
6) Shaders can be compiled to GLSL 3.3 or WebGL.

These extensions are not in Godot's shader language:
1) The `#use mrt` macro can be used to enable Multiple Render Targets in the compiled fragment shader.

"""

from enum import Enum

DEBUG = True

class ShaderDataType(Enum):
    VOID = 1 
    INT = 2 
    FLOAT = 3 
    BOOL = 4 
    UINT = 5
    DOUBLE = 6
    VEC2 = 7 # float
    VEC3 = 8
    VEC4 = 9
    VEC2B = 10 # bool
    VEC3B = 11
    VEC4B = 12
    VEC2I = 13 # int
    VEC3I = 14
    VEC4I = 15
    VEC2U = 16 # uint
    VEC3U = 17
    VEC4U = 18
    VEC2D = 19 # double
    VEC3D = 20
    VEC4D = 21
    MAT22 = 22 # mat2x2 or mat2
    MAT23 = 23 # mat2x3
    MAT24 = 24 # mat2x4
    MAT32 = 25 # mat3x2
    MAT33 = 26 # mat3x3 or mat3
    MAT34 = 27 # mat3x4
    MAT42 = 28 # mat4x2
    MAT43 = 29 # mat4x3
    MAT44 = 30 # mat4x4 or mat4
    STRUCT = 31

SHADER_NAME_TO_ENUM = {
    "void" : ShaderDataType.VOID,
    "int" : ShaderDataType.INT,
    "float" : ShaderDataType.FLOAT,
    "bool" : ShaderDataType.BOOL,
    "uint" : ShaderDataType.UINT,
    "double" : ShaderDataType.DOUBLE,
    "vec2" : ShaderDataType.VEC2,
    "vec3" : ShaderDataType.VEC3,
    "vec4" : ShaderDataType.VEC4,
    "bvec2" : ShaderDataType.VEC2B,
    "bvec3" : ShaderDataType.VEC3B,
    "bvec4" : ShaderDataType.VEC4B,
    "ivec2" : ShaderDataType.VEC2I,
    "ivec3" : ShaderDataType.VEC3I,
    "ivec4" : ShaderDataType.VEC4I,
    "uvec2" : ShaderDataType.VEC2U,
    "uvec3" : ShaderDataType.VEC3U,
    "uvec4" : ShaderDataType.VEC4U,
    "dvec2" : ShaderDataType.VEC2D,
    "dvec3" : ShaderDataType.VEC3D,
    "dvec4" : ShaderDataType.VEC4D,
    "mat2x2" : ShaderDataType.MAT22,
    "mat2x3" : ShaderDataType.MAT23,
    "mat2x4" : ShaderDataType.MAT24,
    "mat3x2" : ShaderDataType.MAT32,
    "mat3x3" : ShaderDataType.MAT33,
    "mat3x4" : ShaderDataType.MAT34,
    "mat4x2" : ShaderDataType.MAT42,
    "mat4x3" : ShaderDataType.MAT43,
    "mat4x4" : ShaderDataType.MAT44,
    "mat2" : ShaderDataType.MAT22,
    "mat3" : ShaderDataType.MAT33,
    "mat4" : ShaderDataType.MAT44,
    "struct" : ShaderDataType.STRUCT,
}

SHADER_ENUM_TO_NAME = {v:k for k,v in SHADER_NAME_TO_ENUM.items()}
# make sure the convenient names are used:
SHADER_ENUM_TO_NAME[ShaderDataType.MAT22] = "mat2"
SHADER_ENUM_TO_NAME[ShaderDataType.MAT33] = "mat3"
SHADER_ENUM_TO_NAME[ShaderDataType.MAT44] = "mat4"

SHADER_TYPENAMES = [k for k,_ in SHADER_NAME_TO_ENUM.items()]

# UNUSED
class ShaderVariable:
    def __init__(self, dtype : ShaderDataType, name : str, value : str = ""):
        self.dtype = dtype 
        self.name = name 
        self.value = value

class UniformMetaData:
    def __init__(self, def_line):
        self.default_value = None
        self.hint_range_min = None 
        self.hint_range_max = None 

        if '=' in def_line:
            eq_ix = def_line.find('=')
            self.default_value = def_line[eq_ix + 1:-1].strip()

        if 'hint_range' in def_line:
            begin_ix = def_line.find('hint_range(') + len('hint_range(')
            comma_ix = def_line.find(',', begin_ix)
            end_ix = def_line.find(')', comma_ix)
            self.hint_range_min = def_line[begin_ix:comma_ix].strip()
            self.hint_range_max = def_line[comma_ix + 1:end_ix].strip()

    def set_hint_range(self, minval : str | None, maxval : str | None):
        self.hint_range_min = minval 
        self.hint_range_max = maxval

class TextureFilter(Enum):
    NEAREST = 1 
    LINEAR = 2

class TextureUniformMetaData:
    def __init__(self, def_line):
        self.is_repeat : bool = True 
        self.filter : TextureFilter = TextureFilter.NEAREST
        if ":" in def_line:
            if 'repeat_disable' in def_line:
                self.is_repeat = False 
            if 'filter_linear' in def_line:
                self.filter = TextureFilter.LINEAR

class Error:
    def __init__(self, message : str = ""):
        self.is_error : bool = False 
        self.message : str = message 
        if self.message:
            self.is_error = True

    def __bool__(self):
        return self.is_error

    def __str__(self):
        return self.message

class ParseState(Enum):
    GLOBAL = 1
    STRUCT = 2 
    FUNC = 3
    MACRO = 4

class ParseBehavior(Enum):
    NORMAL = 1
    SEEK_ACCUMULATE = 3

class ShaderSymbolTracker:
    def __init__(self):
        self.uniform_names : set[str] = set()
        self.constant_names : set[str] = set()
        self.func_names : set[str] = set()
        self.struct_names : set[str] = set()

class ShaderCompiler:
    def __init__(self):
        # internal state:
        self.defines : list[str] = [] 
        self.uniforms_all : dict[str, str] = {} # format is {uniform name: definition}
        self.uniform_hints_normal : dict[str, UniformMetaData] = {}
        self.uniform_hints_texture : dict[str, TextureUniformMetaData] = {}
        self.constants_all : dict[str, str] = {} # format is {variable name: definition}
        self.in_vert : list[str] = [] # parsed from shader
        self.varying : list[str] = [] # variables sent from vertex to fragment
        self.out_frag : str = "" # only supporting one vec4 output
        self.is_default_vertex : bool = True 
        self.is_default_fragment : bool = True
        self.is_mrt : bool = False # using multiple render targets (affects fragment shader)
        self.functions_all : dict[str, str]= {} # format is {function name: definition}
        self.structs_all : dict[str, str] = {} # format is {struct name: definition}
        self.symbol_usage_vert = ShaderSymbolTracker()
        self.symbol_usage_frag = ShaderSymbolTracker()

        # result state 
        self.vertex_code : str = ""
        self.fragment_code  : str = ""
        self.metadata : str = ""

    def compile(self, code : str) -> Error:
        code = self.preprocess(code)
        line_no = 1
        code_len = len(code)

        state = ParseState.GLOBAL
        behavior = ParseBehavior.NORMAL
        seek = None
        buf = ""
        statement_start = 0
        seek_depth = 0
        seek_depth_inc = None
        for i in range(code_len):
            char = code[i]
            if behavior == ParseBehavior.SEEK_ACCUMULATE:
                buf += char
                if seek == char and seek_depth == 0:
                    if state == ParseState.FUNC:
                        self.process_func(buf)
                    elif state == ParseState.STRUCT:
                        self.process_struct(buf)
                    elif state == ParseState.MACRO:
                        self.parse_macro(buf.strip())

                    behavior = ParseBehavior.NORMAL 
                    state = ParseState.GLOBAL
                    buf = ""
                    seek = None
                    seek_depth_inc = None
                    seek_depth = 0
                    statement_start = i + 1
                elif seek_depth_inc == char:
                    seek_depth += 1
                elif seek == char:
                    seek_depth -= 1


            elif state == ParseState.GLOBAL:
                if i == statement_start:
                    # we're at the beginning of a new line. Check if it's a function or struct definition
                    if char == '\n' or char == '\r' or char == '\t' or char == ' ':
                        # handle whitespace by advancing statement_start 
                        statement_start += 1
                    elif char == '#':
                        # macro definition 
                        state = ParseState.MACRO 
                        behavior = ParseBehavior.SEEK_ACCUMULATE
                        seek = '\n'
                        buf += char

                    elif code.startswith("struct", i):
                        state = ParseState.STRUCT 
                        behavior = ParseBehavior.SEEK_ACCUMULATE
                        seek = '}'
                        seek_depth_inc = '{'
                        seek_depth = -1
                        buf += char
                    else:
                        # check if function definition
                        for typename in SHADER_TYPENAMES:
                            if code.startswith(typename, i):
                                next_openparen = code.find('(', i)
                                next_closeparen = code.find(')', i)
                                next_openbrace = code.find('{', i)
                                next_semicolon = code.find(';', i)
                                if next_openparen < next_closeparen and next_closeparen < next_openbrace and (next_openbrace < next_semicolon or next_semicolon == -1):
                                    state = ParseState.FUNC 
                                    behavior = ParseBehavior.SEEK_ACCUMULATE
                                    seek = '}'
                                    seek_depth_inc = '{'
                                    seek_depth = -1
                                    buf += char
                                    break

                elif char == ';':
                    # this is the end of a global line
                    line = code[statement_start:i+1]
                    err = self.parse_global_line(line)
                    if err.is_error:
                        return err
                    statement_start = i+1

            if char == '\n':
                line_no += 1

        self.finalize()

        return Error()

    # preprocesses everything except for macros
    def preprocess(self, code : str) -> str:
        result = ""
        in_multiline_comment = False
        for line in code.split('\n'):
            line = line.strip()

            if line.startswith("//"):
                continue

            comment_ix = line.find('//')
            if comment_ix > -1:
                line = line[:comment_ix]

            if len(line) == 0:
                continue


            # this will fail for something like `int /*this code*/ myVar /* is cheeky */ = 5;`
            if not in_multiline_comment: 
                mlc_ix = line.find("/*")
                if mlc_ix > -1:
                    mlc_end_ix = line.find("*/", mlc_ix + 2)
                    if mlc_end_ix == -1:
                        line = line[:mlc_ix]
                        in_multiline_comment = True
                    else:
                        line = line[:mlc_ix] + line[mlc_end_ix + 2:]
            else:
                mlc_end_ix = line.find("*/")
                if mlc_end_ix > -1:
                    line = line[mlc_end_ix + 2:]
                    in_multiline_comment = False
                else:
                    continue

            if len(line) > 0:
                result += line + '\n'

        return result

    # parses lines which are not inside of {braces}
    def parse_global_line(self, line : str) -> Error:
        if line.startswith("#"):
            self.parse_macro(line)
        elif line.startswith("in "):
            self.parse_invar(line)
        elif line.startswith("out "):
            return self.parse_outvar(line)
        elif line.startswith("uniform"):
            self.parse_uniform(line)
        elif line.startswith("const"):
            return self.parse_constant(line)
        elif line.startswith("varying"):
            self.parse_varying(line)
        else:
            if DEBUG:
                print("ignoring line: ", line)

        return Error()

    def process_struct(self, code : str) -> Error:
        # print("process_struct got:\n", code)
        brace_ix = code.find('{')
        if brace_ix == -1:
            return Error("struct definition is missing a '{'")

        struct_name = code[:brace_ix].strip().split(' ')[1]
        self.structs_all[struct_name] = code
        return Error()

    def process_func(self, code : str) -> Error:
        # print("process_func got:\n", code)
        paren_ix = code.find('(')
        if paren_ix == -1:
            return Error("struct definition is missing a '('")

        func_name = code[:paren_ix].strip().split(' ')[1]
        self.functions_all[func_name] = code
        if self.is_default_vertex and func_name == "vertex":
            self.is_default_vertex = False
        elif self.is_default_fragment and func_name == "fragment":
            self.is_default_fragment = False 
        elif func_name == "main":
            return Error("Cannot have function named `main`. Use `vertex` or `fragment`")
        return Error()

    def parse_macro(self, line : str) -> None:
        # print("parse_macro got:\n", line)
        # TODO extend this macro to use custom variable names, e.g. #use mrt FragColor AllDepth OcclColor OcclDepth
        if line.startswith("#use mrt"):
            self.is_mrt = True 
        else:
            self.defines.append(line)

    def parse_invar(self, line : str) -> None:
        # print("parse_invar got:\n", line)
        self.in_vert.append(line)

    def parse_outvar(self, line : str) -> Error:
        # print("parse_outvar got:\n", line)
        if self.out_frag:
            return Error("Setting `out` line ({line}) but already have {self.out_frag}")
        self.out_frag = line
        return Error()

    def parse_uniform(self, line : str) -> None:
        # print("parse_uniform got:\n", line)
        varname = ''
        decl_line = line
        col_ix = line.find(':')
        if col_ix > -1:
            decl_line = line[:col_ix].strip() + ';'
            varname = decl_line.split(' ')[2][:-1].strip()
        else:
            eq_ix = line.find('=')
            if eq_ix > -1:
                decl_line = line[:eq_ix].strip() + ';'
                varname = decl_line.split(' ')[2][:-1].strip()
            else:
                varname = line.split(' ')[2][:-1].strip()

        if 'sampler2D' in line:
            self.uniform_hints_texture[varname] = TextureUniformMetaData(line)
        else:
            self.uniform_hints_normal[varname] = UniformMetaData(line)

        self.uniforms_all[varname] = decl_line

    def parse_constant(self, line : str) -> Error:
        # print("parse_constant got:\n", line)
        # format is `const dtype varname = value;`
        eq_ix = line.find('=')
        if eq_ix == -1:
            return Error("definition of constant must assign a value")
        varname = line[:eq_ix].strip().split(' ')[2].strip()
        self.constants_all[varname] = line
        return Error()

    def parse_varying(self, line : str) -> None:
        # print("parse_varying got:\n", line)
        self.varying.append(line)

    def get_func_body(self, func) -> str:
        code = self.functions_all[func]
        brace_ix = code.find('{')
        return code[brace_ix:]

    # convenience method to check if `caller_func` calls `query_func`
    def is_func_called_by(self, query_func, caller_func) -> bool:
        # this will have some false positives
        return query_func in self.get_func_body(caller_func)

    # convenience method to check if `caller_func` uses `query_uniform`
    def is_uniform_used_by(self, query_uniform, caller_func) -> bool:
        return query_uniform in self.get_func_body(caller_func)

    def is_constant_used_by(self, query_constant, caller_func) -> bool:
        return query_constant in self.get_func_body(caller_func)

    def is_struct_used_by(self, query_struct, caller_func) -> bool:
        return query_struct in self.get_func_body(caller_func)

    def build_symbol_lists(self, tracker : ShaderSymbolTracker, start_func_name : str) -> None:
        tracker.func_names.add(start_func_name)
        func_queue = [start_func_name]
        # build the list of functions symbols used by this shader
        while len(func_queue) > 0:
            current = func_queue.pop()
            for func_name in self.functions_all.keys():
                if func_name in tracker.func_names:
                    continue # already processed 
                if self.is_func_called_by(func_name, current):
                    tracker.func_names.add(func_name)
                    func_queue.append(func_name)

        # now that we know all the functions used by this shader, we can easily check which uniforms/constants/structs we need 

        for func_name in tracker.func_names:
            for uniform_name in self.uniforms_all.keys():
                if self.is_uniform_used_by(uniform_name, func_name):
                    tracker.uniform_names.add(uniform_name)
                
            for const_name in self.constants_all.keys():
                if self.is_constant_used_by(const_name, func_name):
                    tracker.constant_names.add(const_name)

            for struct_name in self.structs_all.keys():
                if self.is_struct_used_by(struct_name, func_name):
                    tracker.struct_names.add(struct_name)

    def _write_shader_string(self, is_vertex : bool) -> None:
        symbol_tracker : ShaderSymbolTracker = self.symbol_usage_vert if is_vertex else self.symbol_usage_frag
        result = "#version 330\n"
        if is_vertex:
            for thing in self.in_vert:
                result += thing + '\n'
            for thing in self.varying:
                result += thing.replace("varying", "out") + '\n'
        else:
            if self.is_mrt:
                # TODO custom mrt variable names
                result += """
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 AllDepth;
layout(location = 2) out vec4 OcclColor;
layout(location = 3) out vec4 OcclDepth;
                """.strip() + '\n'

            for thing in self.varying:
                result += thing.replace("varying", "in") + '\n'

            if not self.is_mrt:
                result += self.out_frag + '\n'


        for struct_name in symbol_tracker.struct_names:
            result += self.structs_all[struct_name] + '\n'

        for uniform_name in symbol_tracker.uniform_names:
            result += self.uniforms_all[uniform_name] + '\n'

        for const_name in symbol_tracker.constant_names:
            result += self.constants_all[const_name] + '\n'

        for func_name in symbol_tracker.func_names:
            if is_vertex and func_name == "vertex":
                result += self.functions_all[func_name].replace("vertex()", "main()") + '\n'
            elif not is_vertex and func_name == "fragment":
                result += self.functions_all[func_name].replace("fragment()", "main()") + '\n'
            else:
                result += self.functions_all[func_name] + '\n'

        if is_vertex:
            self.vertex_code = result 
        else:
            self.fragment_code = result

    def _write_shader_string_default(self, is_vertex : bool) -> None:
        if is_vertex:
            if self.is_mrt:
                self.vertex_code = "???"# TODO
            else:
                self.vertex_code = """#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
out vec4 fragColor;
void main() {
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}"""
        else:
            raise ValueError("Default fragment shader not implemented")

    # processes all the internal state to generate the new vertex and fragment shaders
    def finalize(self) -> None:
        if not self.is_default_vertex:
            self.build_symbol_lists(self.symbol_usage_vert, "vertex")
            self._write_shader_string(True)
        if not self.is_default_fragment:
            self.build_symbol_lists(self.symbol_usage_frag, "fragment")
            self._write_shader_string(False)

        meta_json = {"sampler2D" : {}, "uniform" : {}}
        for name, metadata in self.uniform_hints_texture.items():
            meta_json["sampler2D"][name] = {'repeat_enable' : str(metadata.is_repeat).lower(), 
                                            'filter' : 'linear' if metadata.filter == TextureFilter.LINEAR else 'nearest'}

        for name, metadata in self.uniform_hints_normal.items():
            meta_json["uniform"][name] = {}
            if metadata.default_value is not None:
                meta_json["uniform"][name]["default_value"] = metadata.default_value

            if metadata.hint_range_min is not None:
                meta_json["uniform"][name]["hint_range_min"] = metadata.hint_range_min

            if metadata.hint_range_max is not None:
                meta_json["uniform"][name]["hint_range_max"] = metadata.hint_range_max

        self.metadata = str(meta_json)

def file_to_string(path : str) -> str:
    s = ""
    with open(path, "r") as f:
        s = f.read()
    return s

if __name__ == "__main__":
    import sys 

    if len(sys.argv) > 1:
        path = sys.argv[1]
        code = file_to_string(path)

        compiler = ShaderCompiler()
        err = compiler.compile(code)
        if err.is_error:
            print("got error: \n", err.message)
        else:
            print("VERTEX CODE:\n", compiler.vertex_code, '\n\n')
            print("FRAGMENT CODE:\n", compiler.fragment_code, '\n\n')
            print("METADATA:\n", compiler.metadata)
