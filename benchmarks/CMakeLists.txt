# benchmarks/CMakeLists.txt
####################################################################
# PROJECT FILES
file(GLOB_RECURSE benchmark_sources 
    "*.c"
    "*.h"
)
####################################################################
# Add executable
add_executable(benchmarks ${benchmark_sources})
####################################################################
# PROJECT SETTINGS
####################################################################
# Link the engine library (either shared or static)
target_link_libraries(benchmarks PRIVATE engine)

# Platform-specific configuration
if(WIN32)
    set_target_properties(benchmarks PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
elseif(APPLE)
    target_link_libraries(benchmarks PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )
elseif(UNIX)
    target_link_libraries(benchmarks PRIVATE m X11)
elseif(ANDROID)
    target_link_libraries(benchmarks PRIVATE native_app_glue ${log-lib} ${android-lib} ${vulkan-lib} volk cglm)
endif()

target_include_directories(benchmarks PUBLIC
    ../engine
    ../
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

# compile options
if(MSVC)
  target_compile_options(benchmarks PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(benchmarks PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()

####################################################################
# Custom target to run the benchmarks
add_custom_target(run.benchmarks
    COMMAND benchmarks
    DEPENDS benchmarks
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running engine benchmarks"
)
####################################################################