cmake_minimum_required(VERSION 3.4.1)

add_library(
        crack # 我们so的名字
        SHARED

        # 包含我们的核心库和破解逻辑
        inlineHook.c
        relocate.c
        native-lib.cpp
)

find_library(
        log-lib
        log
)

target_link_libraries(
        crack
        ${log-lib}
)
