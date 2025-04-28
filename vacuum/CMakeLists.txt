cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(vacuum)

# 添加头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 直接使用预编译的静态库
add_library(vacuum INTERFACE)

# 设置库属性
set_target_properties(vacuum PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接预编译的静态库
target_link_libraries(vacuum INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libvacuum.a)

# 设置安装路径
install(TARGETS vacuum
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
) 