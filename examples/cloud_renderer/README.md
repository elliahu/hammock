# Atmosphere renderer by Matěj Eliáš
This `README.md` is here to help you to:
- Compile and run the code in this repository
- Understand how to navigate the code
- Understand how to use the atmospheric renderer

Please make sure to follow these steps exactly as these were tested to be working on multiple Windows and Linux machines. If there are any issues, feel free to email me. 

**MacOS unfortunately is not supported**

## Vulkan SDK required
The atmosphere renderer uses my own Vulkan engine/wrapper. That means Vulkan SDK version 1.3.296 or newer is **required**. Recommended version that was used during a benchmark is 1.4.309. You can download Vulkan SDK installer from official [LunarG](https://vulkan.lunarg.com/sdk/home) repository. If you already have Vulkan SDK, the installation will install new version alongside the old version and update the PATH to point to the new version so there should be no problem. You can safely delete the old directory containing the old SDK.

**After the installation, make sure VULKAN_SDK environment variable is present.** It may look like this on Windows: `C:\VulkanSDK\1.4.309.0` and on Linux


## Building
### Setup
This is te **required** build environment:
- **Vulkan SDK** version 1.3.296 or newer as noted above, please read.
- **MinGW-w64** version 11.w64 or newer or any toolset that uses `gcc` and `g++` compilers. You can get it from [MSYS2](https://www.msys2.org/), [Winlibs](https://winlibs.com/) or if on Linux, it should be available in your package manager
- **CMake** version 3.29 or newer available [here](https://cmake.org/download/) or if on Linux, in your package manager
- **Ninja-build** as a build tool for CMake available [here](https://ninja-build.org/) or if on Linux, in your package manager

On windows: Make sure all these tools are present in your PATH env variable such as like this:
```
C:\msys64\ucrt64\bin
C:\Program Files\CMake\bin
C:\ninja
```
The first one points where the `gcc.exe` and `g++.exe` compilers, second one points to CMake binary `cmake.exe` and last one points where `ninja.exe` is. 

On Linux: you should be able to just install these tools from the apt (or similar) package manager and that is it.
```
sudo apt update
sudo apt install cmake ninja-build build-essential
```

### Windows
Make sure your **cwd** is the root of the project. Then run the following command:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc.exe -DCMAKE_CXX_COMPILER=g++.exe
```
You should se output similar to this:
```txt
-- The C compiler identification is GNU 14.2.0
-- The CXX compiler identification is GNU 14.2.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/msys64/ucrt64/bin/gcc.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/msys64/ucrt64/bin/g++.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found Vulkan: C:/VulkanSDK/1.4.309.0/Lib/vulkan-1.lib (found version "1.4.309") found components: glslc glslangValidator
-- Configuring done (1.8s)
-- Generating done (0.1s)
-- Build files have been written to: C:/Users/matej/CLionProjects/hammock/build
```
Here it should correctly find the `gcc` and `g++` compilers and Vulkan SDK. 

Then to generate the executable, run this:
```bash
cmake --build build
```
You should see output similar to this:

```txt
... bunch of warnings
[96/96] Linking CXX executable path\to\cloud_renderer.exe
```
If you have troubles building, see Troubleshooting section bellow

### Linux
Here the situation is the same except it is easier to install the build tools.
Make sure your **cwd** is the root of the project. Then run the following command:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc.exe -DCMAKE_CXX_COMPILER=g++.exe
```


## Running
You can run the built executable like this:
```
cmake-build-release/cloud_renderer --width 1920 --height 1080 --scene renderer
```
Required options:
- `--width <value>` horizontal resolution
- `--height <value>` vertical resolution
- `--scene <renderer|medium>` selected scene, cane be one of `renderer` for complete atmospheric renderer or `medium` fro the scene with the Stanford dragon from the theoretical section of the thesis

## Troubleshooting
This might help you if something doesn't work

### 'cmake' is not recognized as an internal or external command, operable program or batch file.
This happens if you either did not install cmake correctly or `bin` folder of the cmake installation location is not in your PATH.

### CMake configuration failed
This may happen in this cases
- You did not install  `gcc` and/or `g++` compilers
-  `gcc` and/or `g++` is not in Your PATH

Make sure you correctly installed MinGW-w64 (not clang or msvc (visual studio) or c1, the app is built for MinGW and will probably fail to build using Visual Studio)

### CMake build failed
If the configuration was successful yet the build failed, it may indicate that wrong build tools (Microsoft's msvc from Visual Studio or clang) were used during the build. Delete the build directory, redo the build steps, and make sure you specify `-DCMAKE_C_COMPILER` and `-DCMAKE_CXX_COMPILER` which point to `gcc` and `g++` binaries respectively.