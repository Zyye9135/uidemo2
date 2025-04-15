set(DATABASE_SOURCES
    dbmanager.cpp
)

set(DATABASE_HEADERS
    dbmanager.h
)

add_library(database STATIC ${DATABASE_SOURCES} ${DATABASE_HEADERS})

target_link_libraries(database
    PRIVATE
    Qt6::Core
    gncdb
)
