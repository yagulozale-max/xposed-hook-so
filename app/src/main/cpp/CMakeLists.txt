cmake_minimum_required(VERSION 3.4.1)

# 定义库，包含所有C/C++源文件
add_library(
        crack
        SHARED
        native-lib.cpp
        inlineHook.c
        relocate.c)

# 查找Android系统日志库
find_library(
        log-lib
        log)

# 查找动态链接库
find_library(
        dl-lib
        dl)

# 将我们的库链接到系统库
target_link_libraries(
        crack
        ${log-lib}
        ${dl-lib})

# 如果有头文件目录，需要包含进来
target_include_directories(crack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)