# This script checks if the system has correct Vulkan version that includes slangc compiler
# Supports both Windows and Linux, MacOS is not supported
# Usage:
# python compile.py source1 output1 entry1 source2 output2 entry2 ...

import os
import subprocess
import platform
import sys

print("Running doctor ...")

# Determine the operating system
if platform.system() == 'Windows':
    compiler = os.path.join(os.environ.get('VULKAN_SDK', ''), 'Bin', 'slangc.exe')
    print("Target system Windows detected")
elif platform.system() == 'Linux':
    compiler = 'slangc'
    print("Target system Linux detected")
else:
    raise EnvironmentError("Unsupported OS. Supported operating systems are: Linux, Windows")

# Check if VULKAN_SDK env var is set
if 'VULKAN_SDK' not in os.environ or not os.path.exists(os.environ['VULKAN_SDK']):
    print("VULKAN_SDK environment variable is not set or path does not exist. Please install Vulkan SDK and relaunch the shell.")
    exit(1)
else:
    print("Found VULKAN_SDK env var")

# Check if the slang compiler exists
if platform.system() == 'Windows' and not os.path.exists(compiler):
    print("Failed to find slang compiler! Make sure Vulkan SDK 1.3.296.0 or newer is installed.")
    exit(1)
else:
    print("Found slangc compiler")

print("OK")

# Check command line arguments
if len(sys.argv) < 4 or (len(sys.argv) - 1) % 3 != 0:
    print("Usage: python compile.py source1 output1 entry1 [source2 output2 entry2 ...]")
    exit(1)

# Compile all shaders from command line arguments
print("Compiling shaders, this may take few seconds...")

for i in range(1, len(sys.argv), 3):
    source_file = sys.argv[i]
    output_file = sys.argv[i+1]
    entry_point = sys.argv[i+2]

    # Ensure output directory exists
    output_dir = os.path.dirname(output_file)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    print(f"Compiling {source_file} -> {output_file} (entry: {entry_point})")
    try:
        subprocess.check_call([compiler, source_file, '-o', output_file, '-target', 'spirv', '-entry', entry_point],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        print(f"Failed to compile {source_file} with entry point {entry_point}")
        exit(1)

print("All shaders compiled successfully!")
