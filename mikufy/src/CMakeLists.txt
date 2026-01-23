cmake_minimum_required(VERSION 3.10)
project(Mikufy VERSION 1.23 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找必要的包
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)

# 源文件
set(SOURCES
    main.cpp
)

# 创建可执行文件
add_executable(mikufy ${SOURCES})

# 包含目录
target_include_directories(mikufy PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTK3_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    /usr/include/webkitgtk-4.0
    /usr/include/webkitgtk-4.0/webkit
    /usr/include/javascriptcoregtk-4.0
    /usr/include/javascriptcoregtk-4.0/JavaScriptCore
    /usr/include/jsc
)

# 链接库
target_link_libraries(mikufy PRIVATE
    ${GTK3_LIBRARIES}
    ${WEBKIT_LIBRARIES}
)

# 编译选项
target_compile_options(mikufy PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${WEBKIT_CFLAGS_OTHER}
    -Wall
    -Wextra
    -Wpedantic
)

# 注意：不使用 CMake 的 install 规则，因为需要 root 权限
# 打包过程在 build.sh 脚本中手动处理
