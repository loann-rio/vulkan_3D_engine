import os
from subprocess import check_output, CalledProcessError

os.chdir(os.path.dirname(os.path.abspath(__file__)))

glsl_compiler_path = r"C:\VulkanSDK\1.4.313.2\Bin\glslc.exe"

def process_mshader(shader_name:str, file_format:str):

    shader = []
    shader = open("shaders\\"+shader_name).read().split("\n")

    if not shader:
        print("failed to open ", shader_name)
        return
    
    for i in range(len(shader)):
        if "#include" in shader[i]:
            file = shader[i][shader[i].find('"')+1 : shader[i].rfind('"')]

            include = open("shader\\"+file)

            shader[i] = include
    
    open("shaders\\"+shader_name).read().split("\n")
    with open("shaders\\temp_"+shader_name[:shader_name.rfind(".")]+) as f:
        f.write("\n".join(shader))
  

def compile_shader(shader_name:str):
    try:

        file_format = shader[shader_name.rfind(".")+1:]

        if (file_format in ["mfrag", "mvert"]):
            return
        
        elif (file_format in ["frag, vert"]):

            check_output([
                glsl_compiler_path,
                f"shaders\\{shader_name}",
                "-o", f"shaders\\{shader_name}.spv",
                "--target-env=vulkan1.2"
            ])



        print("Compiled : ", shader_name)


    except CalledProcessError as e:

        print("Compilation failed:")
        print(e.stderr.decode() if e.stderr else e)


with open("shaders\\_shader_list.txt") as f:
  shader_list = f.read().split()
  
  for shader in shader_list:
      compile_shader(shader)






######## rules ########
#
#     mfrag / mvert
#
#     #include "file.minc"
#
#     def main func -> rename to main
#
#     descriptor set index / binding index
#
#### inputs : ####
#
#   name of main function: string
#
## optional: ##
#   path to shader
#   path to result spv folder