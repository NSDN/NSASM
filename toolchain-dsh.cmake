# DSH Mobile (Android/bionic) 的 CMake toolchain 文件
#
# 重启引擎后 clang 扩展的 CPATH / LIBRARY_PATH 已经配好，只需要处理三处：
#   1) libc++ 头目录必须用 -I 补上，且必须排在 CPATH 之前
#      （CPATH 等价于 -I，命令行 -isystem 排在它后面，会让 bionic 的 <stdio.h>
#        先被选中，libc++ 的 <cstdio> 直接报错）
#   2) CMAKE_SYSTEM_VERSION：CMake 会去读 $PREFIX/include/android/api-level.h，
#      而该文件在 extensions/clang/include 下，读失败会中止 configure
#      -> 请显式传 -DCMAKE_SYSTEM_VERSION=24
#   3) Termux 版 make 把 recipe shell 硬编码成 /data/data/com.termux/files/usr/bin/sh，
#      这台设备上没有该路径，所有构建规则都会 Error 127。make 不读环境变量 SHELL，
#      sub-make 又会丢掉命令行赋值，所以这里在 $HOME 生成一个包装器并交给 CMake。

set(DSH_PREFIX "$ENV{PREFIX}")
if(NOT DSH_PREFIX)
  message(FATAL_ERROR "PREFIX 环境变量未设置")
endif()

set(CLANG_ROOT "${DSH_PREFIX}/extensions/clang")

set(CMAKE_C_COMPILER   "${CLANG_ROOT}/bin/clang")
set(CMAKE_CXX_COMPILER "${CLANG_ROOT}/bin/clang++")
set(CMAKE_AR           "${CLANG_ROOT}/bin/llvm-ar")
set(CMAKE_RANLIB       "${CLANG_ROOT}/bin/llvm-ranlib")

set(CMAKE_CXX_FLAGS_INIT "-I${CLANG_ROOT}/include/c++/v1")

# ---- make 包装器（见上面第 3 点）----
if(NOT CMAKE_MAKE_PROGRAM)
  set(_DSH_MAKE "$ENV{HOME}/bin/dsh-make")
  if(NOT EXISTS "${_DSH_MAKE}")
    file(WRITE "${_DSH_MAKE}"
      "#!/system/bin/sh\nexec ${CLANG_ROOT}/bin/make SHELL=/system/bin/sh \"$@\"\n")
    file(CHMOD "${_DSH_MAKE}"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif()
  set(CMAKE_MAKE_PROGRAM "${_DSH_MAKE}" CACHE FILEPATH "make 包装器（修正 recipe shell）" FORCE)
endif()
