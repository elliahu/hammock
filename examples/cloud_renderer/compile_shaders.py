import os
import subprocess
import platform

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.environ['VULKAN_SDK'] + '\\Bin\\slangc.exe'
elif platform.system() == 'Linux':
    compiler = 'slangc'
else:
    raise EnvironmentError("Unsupported OS. Supported operating systems are: Linux, Windows")

# Check if vulkan env var is set
if not os.path.exists(os.environ['VULKAN_SDK']):
    print("VULKAN_SDK environment variable is not set. Please install Vulkan SDK, relaunch the shell for new changes to take effect and try again.")
    exit(1)

# Check if the slang compiler exits
if platform.system() == 'Windows' and not os.path.exists(compiler):
    print("Failed to compile the shaders. Slang compiler missing! Make sure Vulkan SDK 1.3.296.0 or newer is installed or install it separately.")
    exit(1)

os.makedirs('spv', exist_ok=True)

# Compile the shaders 

# Participating medium scene
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/medium.slang", '-o', 'spv/medium.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])

# Main renderer
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/clouds.slang", '-o', 'spv/clouds_repr.comp.spv', '-target', 'spirv', '-entry', 'computeMain', '-DCLOUD_RENDER_SUBSAMPLE'])
subprocess.check_call([compiler , "shaders/transmittance.slang", '-o', 'spv/transmittance.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/multiplescattering.slang", '-o', 'spv/multiplescattering.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/skyview.slang", '-o', 'spv/skyview.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/aerialperspective.slang", '-o', 'spv/aerialperspective.comp.spv', '-target', 'spirv', '-entry', 'computeMain'])
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/depth.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/depth.frag.spv', '-target', 'spirv', '-entry', 'pixelDepth'])
subprocess.check_call([compiler , "shaders/depth.slang", '-o', 'spv/linear_depth.frag.spv', '-target', 'spirv', '-entry', 'pixelLinearDepth'])
subprocess.check_call([compiler , "shaders/terrain.slang", '-o', 'spv/terrain.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/terrain.slang", '-o', 'spv/terrain.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/sky.slang", '-o', 'spv/sky.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/sky.slang", '-o', 'spv/sky.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/godrays.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/mask.frag.spv', '-target', 'spirv', '-entry', 'pixelMask'])
subprocess.check_call([compiler , "shaders/occlusion.slang", '-o', 'spv/blur.frag.spv', '-target', 'spirv', '-entry', 'pixelBlur'])
subprocess.check_call([compiler , "shaders/compose.slang", '-o', 'spv/compose.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/compose.slang", '-o', 'spv/compose.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])
subprocess.check_call([compiler , "shaders/postprocess.slang", '-o', 'spv/postprocess.vert.spv', '-target', 'spirv', '-entry', 'vertexMain'])
subprocess.check_call([compiler , "shaders/postprocess.slang", '-o', 'spv/postprocess.frag.spv', '-target', 'spirv', '-entry', 'pixelMain'])