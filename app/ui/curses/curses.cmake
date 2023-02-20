file(GLOB GM_UI_CURSES_SRC
        ${CMAKE_SOURCE_DIR}/app/ui/curses/curses-main.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/curses-main.c

        ${CMAKE_SOURCE_DIR}/app/ui/curses/format-print.h
        ${CMAKE_SOURCE_DIR}/app/ui/curses/format-print.c
        )

include_directories(${CMAKE_SOURCE_DIR}/app/ui/curses/)