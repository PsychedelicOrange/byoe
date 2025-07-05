# engine/CMakeLists.txt
####################################################################
# PROJECT FILES
file(GLOB_RECURSE engine_sources
    ${CMAKE_SOURCE_DIR}/engine/**
)
file(GLOB_RECURSE shaders 
    ${CMAKE_SOURCE_DIR}/engine/shaders/*
)

set_source_files_properties(
  ${external_headers}
  ${external_sources}
  PROPERTIES
  COMPILE_FLAGS "-w"
)

# [Source]: https://stackoverflow.com/questions/31966135/cmake-source-group-not-working-correctly-with-hierarchical-project-setup
foreach(FILE ${engine_sources}) 
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    string(REPLACE "/" "\\" GROUP "${GROUP}")

    set(GROUP "engine${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()      
####################################################################
# Add Target
# Create the engine library (DLL or static)
if(BUILD_SHARED_ENGINE)
	add_library(engine SHARED ${engine_sources})
    target_compile_definitions(engine PRIVATE BUILD_SHARED_ENGINE)
else()
    add_library(engine STATIC ${engine_sources})
endif()
####################################################################
# PROJECT SETTINGS
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")
####################################################################
# BUILDING SHADERS
# Skip for CI
option(CI "If building in CI" OFF)


if(NOT CI)
	find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)

	if(NOT GLSLC_EXECUTABLE)
		message(FATAL_ERROR "Could not find glslc. Ensure the Vulkan SDK is installed and VULKAN_SDK is set.")
	endif()

# Function to compile shaders
function(compile_shader SHADER_SRC SHADER_DST)
    add_custom_command(
        OUTPUT ${SHADER_DST}
        COMMAND ${GLSLC_EXECUTABLE}  --target-env=vulkan1.2 ${SHADER_SRC} -o ${SHADER_DST}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling shader: ${SHADER_SRC} output: ${SHADER_DST}"
        VERBATIM
    )
endfunction()

set(COMPILED_SHADERS)
foreach(SHADER ${shaders})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_SOURCE_DIR}/game/shaders_built/${FILE_NAME}.spv)
    compile_shader(${SHADER} ${SHADER_OUTPUT})
    list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})
endif() 
####################################################################
# COMPILATION AND LINKING
# Platform-specific configuration
# Vulkan paths
if(WIN32)
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    include_directories(${VULKAN_SDK}/Include)
    link_directories(${VULKAN_SDK}/Lib)
elseif(APPLE)
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    include_directories(${VULKAN_SDK}/include)
    link_directories(${VULKAN_SDK}/lib)
elseif(UNIX)
    find_library(Vulkan_LIBRARY NAMES vulkan HINTS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    include_directories(/usr/include /usr/local/include)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_include_directories(engine PUBLIC
    core
    scripting
    external
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES> 
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES> 
    $<TARGET_PROPERTY:volk,INTERFACE_INCLUDE_DIRECTORIES> 
)

# Engine linking
target_link_libraries(engine PRIVATE glfw cglm volk)

# export DLL symbols
if(WIN32 AND BUILD_SHARED_ENGINE)
    set_target_properties(engine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# engine compile options
if(MSVC)
  target_compile_options(engine PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()
####################################################################
