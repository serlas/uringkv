cmake_minimum_required(VERSION 3.10)
project(uringKV)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBURING REQUIRED liburing)
pkg_check_modules(ZLIB REQUIRED zlib)
add_executable(uringkv main.c wal.c sst.c index.c metrics.c)
target_link_libraries(uringkv ${LIBURING_LIBRARIES} ${ZLIB_LIBRARIES})
target_include_directories(uringkv PRIVATE /usr/include) # For io_uring.h