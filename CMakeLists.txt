cmake_minimum_required(VERSION 3.16)

project(gncdb_ui2 VERSION 0.1 LANGUAGES CXX)

# 设置编译类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 设置Qt相关选项
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加gncdblib的包含目录
include_directories(${CMAKE_SOURCE_DIR}/gncdblib/include)

# 添加gncdblib的库目录
link_directories(${CMAKE_SOURCE_DIR}/gncdblib/lib)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找Qt包
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)

# 添加子目录
add_subdirectory(database)
add_subdirectory(ui)
add_subdirectory(gncdblib)  # 移到前面确保库先被构建

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/database
    ${PROJECT_SOURCE_DIR}/ui
)

# 定义源文件和资源
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database/dbmanager.cpp
    database/dbmanager.h
    database/sqlresult.h
    resources/resources.qrc
    ui/tabledialog.cpp
    ui/tabledialog.h
)

# 创建可执行文件
add_executable(gncdb_ui2
    ${PROJECT_SOURCES}
)

# 设置目标属性
target_compile_definitions(gncdb_ui2 PRIVATE
    $<$<CONFIG:Debug>:QT_DEBUG>
    $<$<CONFIG:Release>:QT_NO_DEBUG>
)

target_link_libraries(gncdb_ui2 PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    gncdb
    database
    ui
)

# 设置调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(gncdb_ui2 PRIVATE
        -g3
        -ggdb
        -fno-omit-frame-pointer
        -fno-inline
    )
endif()

# 设置应用属性
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.gncdb_ui2)
endif()
set_target_properties(gncdb_ui2 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# 设置安装路径
include(GNUInstallDirs)
install(TARGETS gncdb_ui2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 添加调试信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")