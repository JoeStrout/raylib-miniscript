# Video plugin build settings (runs in the top-level scope; PLUGIN_DIR is set).
#
# Desktop: VP8 decoding needs libvpx, and sound needs libvorbis + libogg, found through
# pkg-config (on macOS: brew install libvpx libvorbis libogg).  Both are optional: without
# them the plugin still builds and `plugins.video.loaded` is 0.
# Web: the browser decodes; web/video.js provides the Module.vpxVideo* functions that
# RVideo.cpp calls.

if(EMSCRIPTEN)
    target_link_options(raylib-miniscript PRIVATE "SHELL:--pre-js ${PLUGIN_DIR}/web/video.js")
else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(MS_VPX QUIET IMPORTED_TARGET vpx)
        pkg_check_modules(MS_VORBIS QUIET IMPORTED_TARGET vorbis ogg)
    endif()
    if(MS_VPX_FOUND)
        target_compile_definitions(raylib-miniscript PRIVATE HAVE_LIBVPX=1)
        target_link_libraries(raylib-miniscript PkgConfig::MS_VPX)   # plain form, like the rest of CMakeLists.txt
        message(STATUS "Plugin 'video': libvpx ${MS_VPX_VERSION}")
        if(MS_VORBIS_FOUND)
            target_compile_definitions(raylib-miniscript PRIVATE HAVE_LIBVORBIS=1)
            target_link_libraries(raylib-miniscript PkgConfig::MS_VORBIS)
            message(STATUS "Plugin 'video': libvorbis found, audio enabled")
        else()
            message(WARNING "Plugin 'video': libvorbis/libogg not found; videos will have no sound")
        endif()
    else()
        message(WARNING "Plugin 'video': libvpx not found; video playback will be unavailable (macOS: brew install libvpx libvorbis libogg)")
    endif()
endif()
