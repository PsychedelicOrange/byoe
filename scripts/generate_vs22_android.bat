@echo off
echo Generating Anrodoid VS22 project files
echo NDK: %ANDROID_NDK%
echo HOME: %ANDROID_HOME%
echo NDK_ROOT: %NDK_ROOT%
echo JAVA_HOME: %JAVA_HOME%

cmake  -G "Visual Studio 16 2019" -A arm64 -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK%/build/cmake/android.toolchain.cmake" -DANDROID_NDK="%ANDROID_NDK%" -DANDROID_ABI="arm64-v8a" -DANDROID_PLATFORM=latest -B ../build/android-arm64-v8a/ ../

PAUSE