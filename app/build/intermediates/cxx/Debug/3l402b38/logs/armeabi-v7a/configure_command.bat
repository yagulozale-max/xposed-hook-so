@echo off
"F:\\AndroidSdk\\cmake\\3.22.1\\bin\\cmake.exe" ^
  "-HF:\\xposedinlinehook\\XposedInlineHook\\app\\src\\main\\cpp" ^
  "-DCMAKE_SYSTEM_NAME=Android" ^
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" ^
  "-DCMAKE_SYSTEM_VERSION=19" ^
  "-DANDROID_PLATFORM=android-19" ^
  "-DANDROID_ABI=armeabi-v7a" ^
  "-DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a" ^
  "-DANDROID_NDK=F:\\AndroidSdk\\ndk\\25.1.8937393" ^
  "-DCMAKE_ANDROID_NDK=F:\\AndroidSdk\\ndk\\25.1.8937393" ^
  "-DCMAKE_TOOLCHAIN_FILE=F:\\AndroidSdk\\ndk\\25.1.8937393\\build\\cmake\\android.toolchain.cmake" ^
  "-DCMAKE_MAKE_PROGRAM=F:\\AndroidSdk\\cmake\\3.22.1\\bin\\ninja.exe" ^
  "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=F:\\xposedinlinehook\\XposedInlineHook\\app\\build\\intermediates\\cxx\\Debug\\3l402b38\\obj\\armeabi-v7a" ^
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=F:\\xposedinlinehook\\XposedInlineHook\\app\\build\\intermediates\\cxx\\Debug\\3l402b38\\obj\\armeabi-v7a" ^
  "-DCMAKE_BUILD_TYPE=Debug" ^
  "-BF:\\xposedinlinehook\\XposedInlineHook\\app\\.cxx\\Debug\\3l402b38\\armeabi-v7a" ^
  -GNinja
