include(${CMAKE_SOURCE_DIR}/app/plugins/input/input.cmake)
include(${CMAKE_SOURCE_DIR}/app/plugins/output/output.cmake)

file(GLOB PLUGINS_SRC
        ${CMAKE_SOURCE_DIR}/app/plugins/input-interface.h
        ${CMAKE_SOURCE_DIR}/app/plugins/output-interface.h

        ${CMAKE_SOURCE_DIR}/app/plugins/interface.h
        ${CMAKE_SOURCE_DIR}/app/plugins/interface.c

        ${INPUT_PLUGINS_SRC} ${OUTPUT_PLUGINS_SRC}
        )