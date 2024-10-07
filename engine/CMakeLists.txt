# engine/CMakeLists.txt

file(GLOB_RECURSE engine_sources 
    "core/*.c"
    "scripting/*.c"
    "external/*.c"
)

file(GLOB_RECURSE engine_headers 
    "core/*.h"
    "scripting/*.h"
    "external/*.h"
)

# Add library based on the BUILD_SHARED_ENGINE option
if(BUILD_SHARED_ENGINE)
    add_library(engine SHARED ${engine_sources} ${engine_headers})
    # Define export settings for the DLL
    target_compile_definitions(engine PRIVATE BUILD_SHARED_ENGINE)
else()
    add_library(engine STATIC ${engine_sources} ${engine_headers})
endif()

target_link_libraries(engine PRIVATE glfw cglm)

# Set output name for the library
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")

# include directories
target_include_directories(engine PUBLIC
    core
    scripting
    external
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES>  # Automatically includes cglm headers
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>   # Automatically includes GLFW headers
)

# Platform-specific settings for exporting symbols on Windows
if(WIN32 AND BUILD_SHARED_ENGINE)
    set_target_properties(engine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()