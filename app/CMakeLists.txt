include(${CMAKE_SOURCE_DIR}/app/common/common.cmake)
include(${CMAKE_SOURCE_DIR}/app/plugins/plugins.cmake)

FILE(GLOB GM_SRC
        ape.h                       ape.c
        browser.h                   browser.c
        buffer.h                    buffer.c
        cache.h                     cache.c
        channelmap.h                channelmap.c
        cmdline.h                   cmdline.c
        cmus.h                      cmus.c
        command_mode.h              command_mode.c
        comment.h                   comment.c
        cue.h                       cue.c
        cue_utils.h                 cue_utils.c
        debug.h                     debug.c
        discid.h                    discid.c
        editable.h                  editable.c
        expr.h                      expr.c
        filters.h                   filters.c
        format_print.h              format_print.c
        gbuf.h                      gbuf.c
        glob.h                      glob.c
        help.h                      help.c
        history.h                   history.c
        http.h                      http.c
        id3.h                       id3.c
        input.h                     input.c
        job.h                       job.c
        keys.h                      keys.c
        keyval.h                    keyval.c
        lib.h                       lib.c
        load_dir.h                  load_dir.c
        locking.h                   locking.c
        mergesort.h                 mergesort.c
        misc.h                      misc.c
        options.h                   options.c
        output.h                    output.c
        pcm.h                       pcm.c
        player.h                    player.c
        play_queue.h                play_queue.c
        pl.h                        pl.c
        rbtree.h                    rbtree.c
        read_wrapper.h              read_wrapper.c
        search_mode.h               search_mode.c
        search.h                    search.c
        server.h                    server.c
        spawn.h                     spawn.c
        tabexp_file.h               tabexp_file.c
        tabexp.h                    tabexp.c
        track_info.h                track_info.c
        track.h                     track.c
        tree.c
        uchar.h                     uchar.c
        u_collate.h                 u_collate.c
        ui_curses.h                 ui_curses.c
        window.h                    window.c
        worker.h                    worker.c
        xstrjoin.h                  xstrjoin.c

        mpris.h                     mpris.c

        file.h                      file.c
        path.h                      path.c
        prog.h                      prog.c
        xmalloc.h                   xmalloc.c

        uchar.h                     uchar.c
        convert.h                   convert.c

        # ok
        global.h
        log.h                       log.c
        utils.h                     utils.c
)

add_executable(graceful-music main.c ${GM_SRC} ${COMMON_SRC} ${PLUGINS_SRC})
target_link_libraries(graceful-music ${GIO_LIBRARIES} -lm -lpthread -lncursesw -ldl -lcddb -lcdio_cdda -lcdio -lmad -lmodplug -lwavpack -lfaad -lavcodec -lpulse -lsystemd)
target_link_directories(graceful-music PUBLIC ${GIO_INCLUDE_DIRS})