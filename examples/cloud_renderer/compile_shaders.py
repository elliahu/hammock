import os
import subprocess
import platform

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.environ['VULKAN_SDK'] + '\\Bin\\slangc.exe'
elif platform.system() == 'Linux':
    compiler = 'slangc'
else:
    raise EnvironmentError("Unsupported OS")

# Compile the shaders
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'atmosphere.vert.spv', '-target spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'atmosphere.frag.spv', '-target spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'clouds.comp.spv', '-target spirv', '-entry', 'computeMain'])