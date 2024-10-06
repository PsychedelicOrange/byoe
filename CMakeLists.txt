cmake_minimum_required(VERSION 3.15)
project(byoe_ghost_asteroids C)

# Collect source files
file(GLOB_RECURSE engine_sources engine/*.c engine/*.h)
file(GLOB_RECURSE game_sources   game/*.c game/*.h)

# Add executable
add_executable(byoe_ghost_asteroids ${engine_sources} ${game_sources})

# Fetch dependencies
include(FetchContent)

# Fetch cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG 1796cc5ce298235b615dc7a4750b8c3ba56a05dd # v0.9.4
)
FetchContent_MakeAvailable(cglm)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8 # Latest stable release
)
FetchContent_MakeAvailable(glfw)

# Compiler options and include directories
target_compile_options(byoe_ghost_asteroids PRIVATE -Wall)
target_include_directories(byoe_ghost_asteroids PRIVATE engine/core engine/external)

# Platform-specific configuration
if(WIN32)
    # Windows-specific setup
    link_directories(engine/external/lib)
    
    target_link_libraries(byoe_ghost_asteroids PRIVATE glfw cglm_headers)
    
    set_target_properties(byoe_ghost_asteroids PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
elseif(APPLE)
    # macOS-specific setup
    target_link_libraries(byoe_ghost_asteroids PRIVATE glfw cglm_headers)
    
    # Required frameworks on macOS
    target_link_libraries(byoe_ghost_asteroids PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )

elseif(UNIX)
    # Linux-specific setup
    target_link_libraries(byoe_ghost_asteroids PRIVATE
        glfw
        cglm_headers
        m    # Math library
        X11  # X11 for GLFW on Linux
    )
endif()

# Custom target to run the executable
add_custom_target(run
    COMMAND byoe_ghost_asteroids
    DEPENDS byoe_ghost_asteroids
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the byoe_ghost_asteroids executable"
)

# Make 'run' the default target
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY ADDITIONAL_MAKE_CLEAN_FILES byoe_ghost_asteroids)
