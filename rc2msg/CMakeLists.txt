cmake_minimum_required(VERSION 3.16)
project(rc2msg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(SOURCES
    rc2msg.cpp
)

set(HEADERS
    rc2msg.h
)

add_library(rc2msg STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(rc2msg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/gncdblib/include
)

target_link_libraries(rc2msg PRIVATE
    Qt6::Core
) 