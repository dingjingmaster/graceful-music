file(GLOB GM_UI_CURSES_SRC
        ${CMAKE_SOURCE_DIR}/app/ui/curses/curses-main.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/curses-main.c

        ${CMAKE_SOURCE_DIR}/app/ui/curses/format-print.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/format-print.c
        )

file(GLOB GM_UI_CURSES_SRC1
        ${CMAKE_SOURCE_DIR}/app/ui/curses/widget.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/widget.c

        ${CMAKE_SOURCE_DIR}/app/ui/curses/main-window.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/main-window.c

        ${CMAKE_SOURCE_DIR}/app/ui/curses/side-favorites.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/side-favorites.c
        )

include_directories(${CMAKE_SOURCE_DIR}/app/ui/curses/)