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
find_program(GLSLC_EXECUTABLE glslc HINTS $ENV{VULKAN_SDK}/Bin $ENV{VULKAN_SDK}/bin)
find_program(SPIRV_CROSS_EXECUTABLE spirv-cross)
find_program(DXC_EXECUTABLE dxc)

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

# Function to compile SPIR-V to HLSL + then CSO (only on Windows)
if(WIN32)
    function(compile_hlsl_from_spv SPV_IN HLSL_OUT CSO_OUT FILE_EXTENSION)
        # Infer shader stage
        if(FILE_EXTENSION STREQUAL "vert")
            set(SHADER_MODEL "vs_6_0")
        elseif(FILE_EXTENSION STREQUAL "frag")
            set(SHADER_MODEL "ps_6_0")
        elseif(FILE_EXTENSION STREQUAL "comp")
            set(SHADER_MODEL "cs_6_0")
        elseif(FILE_EXTENSION STREQUAL "geom")
            set(SHADER_MODEL "gs_6_0")
        elseif(FILE_EXTENSION STREQUAL "tesc")
            set(SHADER_MODEL "hs_6_0")
        elseif(FILE_EXTENSION STREQUAL "tese")
            set(SHADER_MODEL "ds_6_0")
        else()
            message(FATAL_ERROR "Unknown shader file extension: .${FILE_EXTENSION}")
        endif()

        add_custom_command(
            OUTPUT ${HLSL_OUT}
            COMMAND ${SPIRV_CROSS_EXECUTABLE} ${SPV_IN} --output ${HLSL_OUT} --hlsl --shader-model 60
            DEPENDS ${SPV_IN}
            COMMENT "Generating HLSL from SPIR-V: ${SPV_IN} -> ${HLSL_OUT}"
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${CSO_OUT}
            COMMAND ${DXC_EXECUTABLE} -T ${SHADER_MODEL} -E main -Fo ${CSO_OUT} ${HLSL_OUT}
            DEPENDS ${HLSL_OUT}
            COMMENT "Compiling HLSL to CSO: ${HLSL_OUT} -> ${CSO_OUT} (${SHADER_MODEL})"
            VERBATIM
        )
    endfunction()
endif()

set(COMPILED_SHADERS)
foreach(SHADER ${shaders})
    get_filename_component(FILE_NAME     ${SHADER} NAME_WE)
    get_filename_component(FILE_EXT      ${SHADER} EXT)       
    string(SUBSTRING ${FILE_EXT} 1 -1    FILE_EXT_NO_DOT)     

    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_SOURCE_DIR}/game/shaders_built/${FILE_NAME}.spv)
    compile_shader(${SHADER} ${SHADER_OUTPUT})
    list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})

    
    if(WIN32)
        set(HLSL_FILE ${CMAKE_SOURCE_DIR}/game/shaders_built/${FILE_NAME}.hlsl)
        set(CSO_FILE  ${CMAKE_SOURCE_DIR}/game/shaders_built/${FILE_NAME}.cso)
        compile_hlsl_from_spv(${SHADER_OUTPUT} ${HLSL_FILE} ${CSO_FILE} ${FILE_EXT_NO_DOT})
        list(APPEND COMPILED_SHADERS ${HLSL_FILE} ${CSO_FILE})
    endif()
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})
####################################################################
# COMPILATION AND LINKING
# Platform-specific configuration
# Vulkan paths
if(WIN32)
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    include_directories(${VULKAN_SDK}/Include)
    link_directories(${VULKAN_SDK}/Lib)
    target_link_libraries(engine PRIVATE
        d3d12
        dxgi
        dxguid
        d3dcompiler
    )
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
if(WIN32)
target_link_libraries(engine PRIVATE glfw cglm volk TracyClient)
endif()

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
