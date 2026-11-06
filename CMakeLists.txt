# NSASM — NyaSama Assembly Script Module
#
# 在 Android/clang 上构建请用 build-android.sh（它会把源码先拷到 $HOME 再编译，
# 因为源码在 sdcard(FUSE) 上直接编译会慢一个数量级）。
#
# 通用 CMake 用法：
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-dsh.cmake \
#         -DCMAKE_SYSTEM_VERSION=24
#   cmake --build build
cmake_minimum_required(VERSION 3.20)
project(nsasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(nsasm
  Launcher.cpp
  Util.cpp
  funcList.cpp
  paramList.cpp
  ng/Front.cpp
  ng/Interp.cpp
  ng/Calc.cpp
)

target_include_directories(nsasm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# par 指令用 std::thread：bionic 上直接给 -pthread 最省事
target_compile_options(nsasm PRIVATE -pthread -Wall)
target_link_options(nsasm PRIVATE -pthread)

# 一致性测试：sh tests/run.sh <可执行文件>
enable_testing()
add_test(NAME conformance
         COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/run.sh $<TARGET_FILE:nsasm>)
