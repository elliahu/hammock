import os
import subprocess
import platform

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.environ['VULKAN_SDK'] + '\\Bin\\slangc.exe'
elif platform.system() == 'Linux':
    compiler = 'slangc'
else:
    raise EnvironmentError("Unsupported OS. Supported operating systems: Linux, Windows")

# Check if vulkan env var is set
if not os.path.exists(os.environ['VULKAN_SDK']):
    print("VULKAN_SDK environment variable is not set. Please install Vulkan SDK, relaunch the shell for new changes to take effect and try again.")
    exit(1)

# Check if the slang compiler exits
if not os.path.exists(compiler):
    print("Failed to compile the shaders. Slang compiler missing! Make sure Vulkan SDK 1.3.296.0 or newer is installed or install it separately.")
    exit(1)

os.makedirs('spv', exist_ok=True)

# Compile the shaders
# Sky scene
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'spv/atmosphere.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/atmosphere.slang", '-o', 'spv/atmosphere.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/blur.slang", '-o', 'spv/radial.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/blur.slang", '-o', 'spv/radial.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/composition.slang", '-o', 'spv/composition.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/composition.slang", '-o', 'spv/composition.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])

# Participating medium scene
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])