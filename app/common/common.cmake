file(GLOB COMMON_SRC
        ${CMAKE_SOURCE_DIR}/app/common/sf.h
        ${CMAKE_SOURCE_DIR}/app/common/compiler.h

        ${CMAKE_SOURCE_DIR}/app/common/log.h
        ${CMAKE_SOURCE_DIR}/app/common/log.c

        ${CMAKE_SOURCE_DIR}/app/common/prog.h
        ${CMAKE_SOURCE_DIR}/app/common/prog.c

        ${CMAKE_SOURCE_DIR}/app/common/debug.h
        ${CMAKE_SOURCE_DIR}/app/common/debug.c

        ${CMAKE_SOURCE_DIR}/app/common/utils.h
        ${CMAKE_SOURCE_DIR}/app/common/utils.c

        ${CMAKE_SOURCE_DIR}/app/common/xmalloc.h
        ${CMAKE_SOURCE_DIR}/app/common/xmalloc.c

        ${CMAKE_SOURCE_DIR}/app/common/key-value.h
        ${CMAKE_SOURCE_DIR}/app/common/key-value.c

        ${CMAKE_SOURCE_DIR}/app/common/channelmap.h
        ${CMAKE_SOURCE_DIR}/app/common/channelmap.c
        )

include_directories(${CMAKE_SOURCE_DIR}/app/common/)