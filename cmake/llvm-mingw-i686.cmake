# Toolchain for native Windows builds with llvm-mingw, targeting 32-bit x86 so pointer sizes match both
# the original executable and wasm32.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(NOT LLVM_MINGW_ROOT)
  if(DEFINED ENV{LLVM_MINGW_ROOT})
    set(LLVM_MINGW_ROOT "$ENV{LLVM_MINGW_ROOT}")
  else()
    set(LLVM_MINGW_ROOT "E:/retools/llvm-mingw-20261006-ucrt-x86_64")
  endif()
endif()

set(CMAKE_C_COMPILER "${LLVM_MINGW_ROOT}/bin/i686-w64-mingw32-clang.exe")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW_ROOT}/bin/i686-w64-mingw32-clang++.exe")
set(CMAKE_RC_COMPILER "${LLVM_MINGW_ROOT}/bin/i686-w64-mingw32-windres.exe")
set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW_ROOT}/i686-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
