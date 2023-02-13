file(GLOB INPUT_PLUGINS_SRC

        ${CMAKE_SOURCE_DIR}/app/plugins/input/aac.h
        ${CMAKE_SOURCE_DIR}/app/plugins/input/aac.c

        # 字幕
#        ${CMAKE_SOURCE_DIR}/app/plugins/input/bass.h
#        ${CMAKE_SOURCE_DIR}/app/plugins/input/bass.c

        ${CMAKE_SOURCE_DIR}/app/plugins/input/flac.h
        ${CMAKE_SOURCE_DIR}/app/plugins/input/flac.c

        ${CMAKE_SOURCE_DIR}/app/plugins/input/ffmpeg.h
        ${CMAKE_SOURCE_DIR}/app/plugins/input/ffmpeg.c
        )

include_directories(${CMAKE_SOURCE_DIR}/app/plugins/)
include_directories(${CMAKE_SOURCE_DIR}/app/plugins/input/)
