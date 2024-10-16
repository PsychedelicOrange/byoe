# engine/CMakeLists.txt

file(GLOB_RECURSE engine_sources 
    "core/*.c"
    "scripting/*.c"
    "external/*.c"
)

file(GLOB_RECURSE engine_headers 
    "core/*.h"
    "scripting/*.h"
)
file(GLOB_RECURSE external_headers
    "external/*.h"
)
file(GLOB_RECURSE external_sources
    "external/*.c"
)

set_source_files_properties(
  ${external_headers}
  ${external_sources}
  PROPERTIES
  COMPILE_FLAGS "-w"
)

SOURCE_GROUP(engine FILES ${engine_sources} ${engine_headers})

# Set the C standard for this project
set(CMAKE_C_STANDARD 11)   # Set C standard (11, 99, 90, etc.)
set(CMAKE_C_STANDARD_REQUIRED ON)  # Ensure the standard is required
set(CMAKE_C_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Add library based on the BUILD_SHARED_ENGINE option
if(BUILD_SHARED_ENGINE)
	add_library(engine SHARED ${engine_sources} ${engine_headers} ${external_headers} ${external_sources})
    # Define export settings for the DLL
    target_compile_definitions(engine PRIVATE BUILD_SHARED_ENGINE)
else()
    add_library(engine STATIC ${engine_sources} ${engine_headers} ${external_headers} ${external_sources})
endif()

target_link_libraries(engine PRIVATE glfw cglm)

# Set output name for the library
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")
set_target_properties(engine PROPERTIES C_STANDARD 11)

# include directories
target_include_directories(engine PUBLIC
    core
    scripting
    external
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

# Platform-specific settings for exporting symbols on Windows
if(WIN32 AND BUILD_SHARED_ENGINE)
    set_target_properties(engine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

if(MSVC)
  target_compile_options(engine PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()
