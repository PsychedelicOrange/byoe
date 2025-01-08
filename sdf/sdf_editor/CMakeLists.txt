
# sdf_editor/CMakeLists.txt

file(GLOB_RECURSE editor_source 
    "*.c"
    "*.h"
)

# Set the C standard for this project
set(CMAKE_C_STANDARD 11)   # Set C standard (11, 99, 90, etc.)
set(CMAKE_C_STANDARD_REQUIRED ON)  # Ensure the standard is required
set(CMAKE_C_EXTENSIONS OFF)  # Disable compiler-specific extensions

####################################################################
# Add executable
add_executable(sdf_editor ${editor_source})
####################################################################
# Link the engine library (either shared or static)
target_link_libraries(sdf_editor PRIVATE engine)
####################################################################
# Platform-specific configuration
if(WIN32)
    set_target_properties(sdf_editor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
elseif(APPLE)
    target_link_libraries(sdf_editor PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )

set_target_properties(engine PROPERTIES C_STANDARD 11)

elseif(UNIX)
    target_link_libraries(sdf_editor PRIVATE m X11)
endif()

target_include_directories(sdf_editor PUBLIC
    ../../engine
    ../../
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

if(MSVC)
  target_compile_options(sdf_editor PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(sdf_editor PRIVATE )
endif()
