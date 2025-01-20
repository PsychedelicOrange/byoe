# engine/CMakeLists.txt


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

find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "Could not find glslc. Ensure the Vulkan SDK is installed and VULKAN_SDK is set.")
endif()

# Function to compile shaders
function(compile_shader SHADER_SRC SHADER_DST)
    add_custom_command(
        OUTPUT ${SHADER_DST}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SHADER_DST}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling shader: ${SHADER_SRC}"
        VERBATIM
    )
endfunction()

set(COMPILED_SHADERS)
foreach(SHADER ${shaders})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv)
    compile_shader(${SHADER} ${SHADER_OUTPUT})
    list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})

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

# [Source]: https://stackoverflow.com/questions/31966135/cmake-source-group-not-working-correctly-with-hierarchical-project-setup
foreach(FILE ${engine_sources}) 
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    string(REPLACE "/" "\\" GROUP "${GROUP}")

    set(GROUP "engine${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

set(CMAKE_C_STANDARD 11)          
set(CMAKE_C_STANDARD_REQUIRED ON) 
set(CMAKE_C_EXTENSIONS OFF)       

if(BUILD_SHARED_ENGINE)
	add_library(engine SHARED ${engine_sources})
    target_compile_definitions(engine PRIVATE BUILD_SHARED_ENGINE)
else()
    add_library(engine STATIC ${engine_sources})
endif()

target_link_libraries(engine PRIVATE glfw cglm volk)
set_target_properties(engine PROPERTIES OUTPUT_NAME "engine")
set_target_properties(engine PROPERTIES C_STANDARD 11)

target_include_directories(engine PUBLIC
    core
    scripting
    external
    $<TARGET_PROPERTY:cglm,INTERFACE_INCLUDE_DIRECTORIES> 
    $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES> 
    $<TARGET_PROPERTY:volk,INTERFACE_INCLUDE_DIRECTORIES> 
)


if(WIN32 AND BUILD_SHARED_ENGINE)
    set_target_properties(engine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

if(MSVC)
  target_compile_options(engine PRIVATE /W4 /WX /wd4996)
else()
  target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
endif()
