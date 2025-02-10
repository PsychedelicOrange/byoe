cmake_minimum_required(VERSION 3.15)
# PROJECT NAME
project(byoe_ghost_asteroids C)
####################################################################
## OPTIONS
# Options to build the engine as DLL or static library
option(BUILD_SHARED_ENGINE "Build engine as shared library (DLL)" OFF)
####################################################################
## GLOBAL COMPILATION SETTINGS
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_C_STANDARD 11)   
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
####################################################################
# GLOBAL PER PLATFORM SETTINGS

##### ANDROID #####
if(ANDROID)
    message(STATUS "Generating for Android...")

    include("${ANDROID_NDK}/build/cmake/android.toolchain.cmake")
    include("$ENV{ANDROID_CMAKE_HOME}/android.apk.cmake" REQUIRED)
    
    set(CMAKE_SYSTEM_NAME Android)
    set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
    set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME})
    set(CMAKE_ANDROID_STL c++_static)

    # find android libs
    find_library(log-lib log)
    find_library(android-lib android)
    find_library(vulkan-lib vulkan)

     # NativeActivity for Android (replaces GLFW)
    add_library(native_app_glue STATIC
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
    target_include_directories(native_app_glue PRIVATE
        ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue
    )

endif()

##### WINDOWS #####
# VS edit and continue for hot reloading
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") 
    add_compile_options("/ZI")
    add_link_options("/INCREMENTAL")
endif()
####################################################################
## SOURCES
# Add subdirectories for engine and game
add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(tests)
add_subdirectory(benchmarks)
####################################################################
## VENDOR
# Fetch dependencies
include(FetchContent)
##############################
## Vulkan
find_package(Vulkan REQUIRED)
set(VULKAN_SDK $ENV{VULKAN_SDK})
##############################
# Fetch cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG 1796cc5ce298235b615dc7a4750b8c3ba56a05dd # v0.9.4
)
# Set options to build cglm as static
set(CGLM_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cglm)
##############################
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
##############################
# Volk header
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG vulkan-sdk-1.3.275.0
)
FetchContent_MakeAvailable(volk)
####################################################################