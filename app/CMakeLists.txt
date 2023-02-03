FILE(GLOB GM_SRC
        global.h
        input-interface.h
        log.h                   log.c
        cache.h                 cache.c
        utils.h                 utils.c
        input.h                 input.c
        key-value.h             key-value.c
        channel-map.h           channel-map.c
)

add_executable(graceful-music main.c ${GM_SRC})
target_link_libraries(graceful-music ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_include_directories(graceful-music PUBLIC ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})