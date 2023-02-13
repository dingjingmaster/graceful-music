file(GLOB OUTPUT_PLUGINS_SRC
        ${CMAKE_SOURCE_DIR}/app/plugins/output/alsa.h
        ${CMAKE_SOURCE_DIR}/app/plugins/output/alsa.c
        ${CMAKE_SOURCE_DIR}/app/plugins/output/mixer-alsa.h
        ${CMAKE_SOURCE_DIR}/app/plugins/output/mixer-alsa.c
)

include_directories(${CMAKE_SOURCE_DIR}/app/plugins/)
include_directories(${CMAKE_SOURCE_DIR}/app/plugins/output/)
