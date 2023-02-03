project(demo C)

FILE(GLOB SIMPLE_MUSIC_PLAYER_SRC
        ${CMAKE_SOURCE_DIR}/app/player-control.h
        ${CMAKE_SOURCE_DIR}/app/player-control.c

        ${CMAKE_SOURCE_DIR}/app/player.h
        ${CMAKE_SOURCE_DIR}/app/player.c
)

add_executable(simple-music-player simple-music-player.c ${SIMPLE_MUSIC_PLAYER_SRC})
target_link_libraries(simple-music-player ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_include_directories(simple-music-player PUBLIC ${CMAKE_SOURCE_DIR}/app ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
