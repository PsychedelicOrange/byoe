cmake_minimum_required(VERSION 3.15)

project(byoe_ghost_asteroids C)

# Collect source files
file(GLOB_RECURSE engine      engine/*.c engine/*.h)
file(GLOB_RECURSE game        game/*.c game/*.h)

if(WIN32)
# link libraries for windows
link_directories(engine/external/lib)
endif()
# Add executable
add_executable(byoe_ghost_asteroids ${engine} ${game})

# Fetch dependencies
include(FetchContent)
FetchContent_Declare(
	cglm
	GIT_REPOSITORY https://github.com/recp/cglm.git
	GIT_TAG 1796cc5ce298235b615dc7a4750b8c3ba56a05dd # 0.9.4
)
FetchContent_MakeAvailable(cglm)

# Add custom target to run the executable
add_custom_target(run
    COMMAND byoe_ghost_asteroids
    DEPENDS byoe_ghost_asteroids
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "Running the byoe_ghost_asteroids executable"
)

# Make 'run' the default target
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY ADDITIONAL_MAKE_CLEAN_FILES byoe_ghost_asteroids)

target_compile_options(byoe_ghost_asteroids PRIVATE -Wall )
target_include_directories(byoe_ghost_asteroids PRIVATE engine/core engine/external )

if(UNIX)
target_link_libraries(
	byoe_ghost_asteroids
	PRIVATE
	glfw3
	X11 # for glfw3 linux
	m # math
	cglm_headers
	)
else()
set_target_properties(byoe_ghost_asteroids PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
target_link_libraries(
	byoe_ghost_asteroids
	PRIVATE
	glfw3
	cglm_headers
	)
endif()

