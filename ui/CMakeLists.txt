set(UI_SOURCES
    tabledialog.cpp
)

set(UI_HEADERS
    tabledialog.h
)

add_library(ui STATIC ${UI_SOURCES} ${UI_HEADERS})

target_link_libraries(ui
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    database
)
