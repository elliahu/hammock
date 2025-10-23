# Define the directory to watch for .slang files
set(SLANG_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# 1. Find all .slang files in the specified directory
file(GLOB_RECURSE SLANG_SOURCES
    "${SLANG_DIR}/*.slang"
)

# Check if any files were found
if(NOT SLANG_SOURCES)
    message(STATUS "No .slang files found in ${SLANG_DIR}")
endif()

# Define the output file/marker file for the command
set(SLANG_OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/slang_processed_marker")

# Define the Python interpreter and script
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(PROCESS_SCRIPT "${PROJECT_SOURCE_DIR}/tools/build_shaders_slang.py")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/spv")


# 2. Add the custom command
# This command runs the python script
add_custom_command(
    OUTPUT ${SLANG_OUTPUT_FILE} 
    COMMAND ${Python3_EXECUTABLE} ${PROCESS_SCRIPT}
             ${SLANG_DIR}/fullscreen.slang ${OUTPUT_DIR}/fullscreen.frag.spv pixelMain
    # This is the crucial part: it forces the command to re-run
    # if any of these files (the .slang sources) have changed.
    DEPENDS ${PROCESS_SCRIPT} ${SLANG_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Processing ${SLANG_DIR} .slang files..."
)


# 3. Add the custom target
# This creates a buildable target that depends on the output of the custom command
add_custom_target(process_slang_files
    ALL # Optional: 'ALL' makes it run during a standard build
    DEPENDS ${SLANG_OUTPUT_FILE}
)

