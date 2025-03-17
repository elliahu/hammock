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

os.makedirs('spv', exist_ok=True)

# Compile the shaders
# Sky scene
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'spv/atmosphere.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'spv/atmosphere.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/composition.slang", '-o', 'spv/composition.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/composition.slang", '-o', 'spv/composition.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])

# Participating medium scene
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])