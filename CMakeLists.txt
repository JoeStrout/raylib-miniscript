cmake_minimum_required(VERSION 3.12)
project(raylib-miniscript VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MiniScript source files
set(MINISCRIPT_SOURCES
    MiniScript/Dictionary.cpp
    MiniScript/List.cpp
    MiniScript/MiniscriptInterpreter.cpp
    MiniScript/MiniscriptIntrinsics.cpp
    MiniScript/MiniscriptKeywords.cpp
    MiniScript/MiniscriptLexer.cpp
    MiniScript/MiniscriptParser.cpp
    MiniScript/MiniscriptTAC.cpp
    MiniScript/MiniscriptTypes.cpp
    MiniScript/QA.cpp
    MiniScript/SimpleString.cpp
    MiniScript/SimpleVector.cpp
    MiniScript/SplitJoin.cpp
    MiniScript/UnicodeUtil.cpp
    MiniScript/UnitTest.cpp
)

# Application source files
set(APP_SOURCES
    src/main.cpp
    src/loadfile.cpp
    src/RaylibIntrinsics.cpp
    src/RaylibTypes.cpp
    src/RawData.cpp
    src/RAudio.cpp
    src/RCore.cpp
    src/RMath.cpp
    src/RModels.cpp
    src/RShapes.cpp
    src/RText.cpp
    src/RTextures.cpp
    src/RaylibConstants.cpp
    src/FileModule.cpp
    src/MoreIntrinsics.cpp
    src/HttpModule.cpp
)

# Create executable
add_executable(raylib-miniscript
    ${APP_SOURCES}
    ${MINISCRIPT_SOURCES}
)

# Include directories (common to all platforms)
target_include_directories(raylib-miniscript PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/MiniScript
)

if(EMSCRIPTEN)
    #---------------------------------------------------------------------------
    # Web build (Emscripten)
    #---------------------------------------------------------------------------
    set(RAYLIB_WEB_LIB ${CMAKE_SOURCE_DIR}/raylib/src/libraylib.web.a)

    if(NOT EXISTS ${RAYLIB_WEB_LIB})
        message(FATAL_ERROR "Raylib web library not found at ${RAYLIB_WEB_LIB}. Please build raylib for web first.")
    endif()

    target_include_directories(raylib-miniscript PRIVATE ${CMAKE_SOURCE_DIR}/raylib/src)

    target_compile_options(raylib-miniscript PRIVATE
        -Os
        -Wall
        -DPLATFORM_WEB
        -fexceptions
    )

    target_link_options(raylib-miniscript PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sFETCH=1
        -sALLOW_MEMORY_GROWTH=1
        -sTOTAL_MEMORY=67108864
        -sSTACK_SIZE=5242880
        -fexceptions
        -sEXPORTED_FUNCTIONS=['_main','_malloc']
        --pre-js ${CMAKE_SOURCE_DIR}/web/prevent-defaults.js
    )

    target_link_libraries(raylib-miniscript ${RAYLIB_WEB_LIB})

    set_target_properties(raylib-miniscript PROPERTIES SUFFIX ".html")

    if(EXISTS ${CMAKE_SOURCE_DIR}/web/shell.html)
        target_link_options(raylib-miniscript PRIVATE --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html)
    endif()

    # Copy assets to build directory
    add_custom_command(TARGET raylib-miniscript POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copying assets to build directory"
    )

    # Copy index.html to build directory
    if(EXISTS ${CMAKE_SOURCE_DIR}/web/index.html)
        add_custom_command(TARGET raylib-miniscript POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/web/index.html
            ${CMAKE_BINARY_DIR}/index.html
            COMMENT "Copying index.html to build directory"
        )
    endif()

    message(STATUS "raylib-miniscript Configuration: Web (Emscripten)")
    message(STATUS "  Raylib library: ${RAYLIB_WEB_LIB}")
    message(STATUS "  Output: raylib-miniscript.html, .js, .wasm")

else()
    #---------------------------------------------------------------------------
    # Desktop build (Mac/Win/Linux)
    #---------------------------------------------------------------------------
    target_compile_options(raylib-miniscript PRIVATE
        -O2
        -Wall
        -DPLATFORM_DESKTOP
        -fexceptions
    )

    # Find raylib - try the symlinked source first, then system install
    set(RAYLIB_LIB_PATH ${CMAKE_SOURCE_DIR}/raylib/src/libraylib.a)

    if(EXISTS ${RAYLIB_LIB_PATH})
        # Use locally built raylib (library + headers from same source)
        target_include_directories(raylib-miniscript PRIVATE ${CMAKE_SOURCE_DIR}/raylib/src)
        target_link_libraries(raylib-miniscript ${RAYLIB_LIB_PATH})
        message(STATUS "Using local raylib: ${RAYLIB_LIB_PATH}")
    else()
        # Try find_package as fallback (uses system headers + library)
        find_package(raylib QUIET)
        if(raylib_FOUND)
            target_link_libraries(raylib-miniscript raylib)
            message(STATUS "Using system raylib")
        else()
            message(FATAL_ERROR "raylib not found. Either:\n"
                "  1. Build raylib for desktop: cd raylib/src && make\n"
                "  2. Install raylib system-wide")
        endif()
    endif()

    # Platform-specific libraries
    if(APPLE)
        target_link_libraries(raylib-miniscript
            "-framework IOKit"
            "-framework Cocoa"
            "-framework OpenGL"
        )
        # libcurl is a macOS system library — no download, zero binary bloat.
        find_package(CURL REQUIRED)
        target_link_libraries(raylib-miniscript CURL::libcurl)
    elseif(UNIX)
        find_package(CURL REQUIRED)
        target_link_libraries(raylib-miniscript
            CURL::libcurl
            GL
            m
            pthread
            dl
            rt
            X11
        )
    elseif(WIN32)
        target_link_libraries(raylib-miniscript
            opengl32
            gdi32
            winmm
            winhttp  # WinHTTP is built into Windows; SChannel provides TLS
        )
    endif()

    # Copy assets to build directory
    add_custom_command(TARGET raylib-miniscript POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copying assets to build directory"
    )

    message(STATUS "raylib-miniscript Configuration: Desktop")
    message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
endif()
