# game/CMakeLists.txt

file(GLOB_RECURSE game_sources 
    "*.c"
    "*.h"
)

# Set the C standard for this project
set(CMAKE_C_STANDARD 99)   # Set C standard (11, 99, 90, etc.)
set(CMAKE_C_STANDARD_REQUIRED ON)  # Ensure the standard is required
set(CMAKE_C_EXTENSIONS OFF)  # Disable compiler-specific extensions

####################################################################
# Add executable
add_executable(byoe_ghost_asteroids ${game_sources})
####################################################################
# Link the engine library (either shared or static)
target_link_libraries(byoe_ghost_asteroids PRIVATE engine)
####################################################################
# Platform-specific configuration
if(WIN32)
    set_target_properties(byoe_ghost_asteroids PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
elseif(APPLE)
    target_link_libraries(byoe_ghost_asteroids PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )

set_target_properties(engine PROPERTIES C_STANDARD 11)

elseif(UNIX)
    target_link_libraries(byoe_ghost_asteroids PRIVATE m X11)
endif()

target_include_directories(byoe_ghost_asteroids PUBLIC
    ../engine
    ../
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

if(MSVC)
  target_compile_options(byoe_ghost_asteroids PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(byoe_ghost_asteroids PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()