file(GLOB INPUT_PLUGINS_SRC
        ${CMAKE_SOURCE_DIR}/app/plugins/input/flac.h
        ${CMAKE_SOURCE_DIR}/app/plugins/input/flac.c

        )
include_directories(${CMAKE_SOURCE_DIR}/app/plugins/)
include_directories(${CMAKE_SOURCE_DIR}/app/plugins/input/)
