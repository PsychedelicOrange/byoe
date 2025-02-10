# tests/CMakeLists.txt
####################################################################
# PROJECT FILES
file(GLOB_RECURSE test_sources 
    "*.c"
    "*.h"
)
####################################################################
# Add executable
add_executable(tests ${test_sources})
####################################################################
# PROJECT SETTINGS
####################################################################
# COMPILATION AND LINKING
# Link the engine library (either shared or static)
target_link_libraries(tests PRIVATE engine)

# Platform-specific configuration
if(WIN32)
    set_target_properties(tests PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
elseif(APPLE)
    target_link_libraries(tests PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework OpenGL"
    )
elseif(UNIX)
    target_link_libraries(tests PRIVATE m X11)
elseif(ANDROID)
    target_link_libraries(tests PRIVATE native_app_glue ${log-lib} ${android-lib} ${vulkan-lib} volk cglm)
endif()

target_include_directories(tests PUBLIC
    ../engine
    ../
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

# tests compile options
if(MSVC)
  target_compile_options(tests PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()

####################################################################
# Custom target to run the tests
add_custom_target(run.tests
    COMMAND tests
    DEPENDS tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running engine tests"
)
####################################################################