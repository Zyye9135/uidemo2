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
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Sql
    REQUIRED
)

# 添加子目录
add_subdirectory(database)
add_subdirectory(ui)

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/database
    ${PROJECT_SOURCE_DIR}/ui
)

# 添加源文件
set(SOURCES
    main.cpp
    mainwindow.cpp
    database/dbmanager.cpp
    ui/tabledialog.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gncdb_ui2_autogen/mocs_compilation.cpp
)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    database/dbmanager.cpp
    database/dbmanager.h
    ui/tabledialog.cpp
    ui/tabledialog.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(gncdb_ui2
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET gncdb_ui2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(gncdb_ui2 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(gncdb_ui2
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# 添加 gncdblib 子目录
add_subdirectory(gncdblib)

# 设置目标属性
target_compile_definitions(gncdb_ui2 PRIVATE
    $<$<CONFIG:Debug>:QT_DEBUG>
    $<$<CONFIG:Release>:QT_NO_DEBUG>
)

target_link_libraries(gncdb_ui2 PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets
    gncdb
    database  # 添加 database 库
    ui        # 添加 ui 库
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql
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

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(gncdb_ui2)
endif()

# 添加调试信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
