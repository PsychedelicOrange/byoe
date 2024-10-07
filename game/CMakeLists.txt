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