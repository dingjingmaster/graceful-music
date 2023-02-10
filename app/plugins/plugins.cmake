include(${CMAKE_SOURCE_DIR}/app/plugins/input/input.cmake)

file(GLOB PLUGINS_SRC
        ${CMAKE_SOURCE_DIR}/app/plugins/input-interface.h

        ${CMAKE_SOURCE_DIR}/app/plugins/interface.h
        ${CMAKE_SOURCE_DIR}/app/plugins/interface.c

        ${INPUT_PLUGINS_SRC}
        )