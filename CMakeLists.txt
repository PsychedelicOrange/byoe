cmake_minimum_required(VERSION 3.15)

project(byoe_ghost_asteroids C)

# Collect source files
file(GLOB_RECURSE engine      engine/*.c engine/*.h)
file(GLOB_RECURSE game        game/*.c game/*.h)

# Add executable
add_executable(byoe_ghost_asteroids ${engine} ${game})

# Add custom target to run the executable
add_custom_target(run
    COMMAND byoe_ghost_asteroids
    DEPENDS byoe_ghost_asteroids
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
    COMMENT "Running the byoe_ghost_asteroids executable"
)

# Make 'run' the default target
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY ADDITIONAL_MAKE_CLEAN_FILES byoe_ghost_asteroids)
