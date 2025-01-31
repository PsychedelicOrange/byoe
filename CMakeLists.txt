cmake_minimum_required(VERSION 3.15)
project(byoe_ghost_asteroids C)

####################################################################
## OPTIONS
# Options to build the engine as DLL or static library
option(BUILD_SHARED_ENGINE "Build engine as shared library (DLL)" OFF)
# generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
####################################################################
## GLOBAL COMPILATION SETTINGS
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# VS edit and continue for h0to reloading
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") 
    add_compile_options("/ZI")
    add_link_options("/INCREMENTAL")
endif()
####################################################################
## SOURCES
# Add subdirectories for engine and game
add_subdirectory(engine)
add_subdirectory(sdf/sdf_editor)
add_subdirectory(game)
add_subdirectory(tests)
add_subdirectory(benchmarks)
####################################################################
## VENDOR
# Fetch dependencies
include(FetchContent)

# Fetch cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG 1796cc5ce298235b615dc7a4750b8c3ba56a05dd # v0.9.4
)
# Set options to build cglm as static
set(CGLM_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cglm)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8 # Latest stable release
    
)
# Disable building GLFW tests and examples
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch Nuklear
FetchContent_Declare(
    Nuklear
    GIT_REPOSITORY https://github.com/Immediate-Mode-UI/Nuklear
    GIT_TAG f09fe4945d5061da345bf7339b6c788e5bb26ebc # v4.09.1
)


####################################################################
# Custom target to run the executable
add_custom_target(run
    COMMAND byoe_ghost_asteroids
    DEPENDS byoe_ghost_asteroids
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the byoe_ghost_asteroids executable"
)
