project(demo C)

FILE(GLOB SIMPLE_MUSIC_PLAYER_SRC

)

add_executable(simple-music-player simple-music-player.c ${SIMPLE_MUSIC_PLAYER_SRC})
target_link_libraries(simple-music-player ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_include_directories(simple-music-player PUBLIC ${CMAKE_SOURCE_DIR}/app ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})

include(${CMAKE_SOURCE_DIR}/app/ui/curses/curses.cmake)
add_executable(curses-main-window curses-main-window.c ${GM_UI_CURSES_SRC1})
target_link_libraries(curses-main-window ${GLIB_LIBRARIES} ${GIO_LIBRARIES} ${CURSES_LIBRARIES})
target_include_directories(curses-main-window PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} ${CURSES_INCLUDE_DIRS})
