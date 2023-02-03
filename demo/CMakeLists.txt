project(demo C)

add_executable(simple-music-player simple-music-player.c)
target_link_libraries(simple-music-player ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_include_directories(simple-music-player PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
