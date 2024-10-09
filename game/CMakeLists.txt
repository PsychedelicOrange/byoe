# game/CMakeLists.txt

file(GLOB_RECURSE game_sources 
    "*.c"
    "*.h"
)
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
  target_compile_options(byoe_ghost_asteroids PRIVATE /W4 /WX)
else()
  target_compile_options(byoe_ghost_asteroids PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-strict-prototypes -Wno-newline-eof -Wno-unused-function)
endif()