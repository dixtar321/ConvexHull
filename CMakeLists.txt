cmake_minimum_required(VERSION 3.19)
project(Task3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Concurrent)

set(HEADERS
    mainwindow.h
    convexhullwidget.h
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    convexhullwidget.cpp
)

add_executable(Task3
    ${HEADERS}
    ${SOURCES}
)

set_target_properties(Task3 PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(Task3
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Concurrent
)

target_compile_definitions(Task3 PRIVATE BUILD_GUI)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS
        ${Qt5_DIR}/../../../bin
        ENV PATH
    )

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET Task3 POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --compiler-runtime
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --dir "$<TARGET_FILE_DIR:Task3>"
                "$<TARGET_FILE:Task3>"
            COMMENT "Running windeployqt..."
        )

        add_custom_command(TARGET Task3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Task3>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${Qt5_DIR}/../../../plugins/platforms/qwindows.dll"
                "$<TARGET_FILE_DIR:Task3>/platforms/qwindows.dll"
            COMMENT "Copying Qt platform plugins..."
        )
    else()
        message(WARNING "windeployqt not found. Please install it or set the path manually.")
    endif()
endif()

if(WIN32 AND MINGW)
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    set(_GCC_DLL_CANDIDATES
        "libgcc_s_seh-1.dll"
        "libgcc_s_dw2-1.dll"
        "libgcc_s_sjlj-1.dll"
    )

    set(GCC_DLL "")
    foreach(cand ${_GCC_DLL_CANDIDATES})
        if(EXISTS "${MINGW_BIN_DIR}/${cand}")
            set(GCC_DLL "${cand}")
            break()
        endif()
    endforeach()

    set(MINGW_RUNTIME_DLLS
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
        ${GCC_DLL}
    )

    foreach(dll ${MINGW_RUNTIME_DLLS})
        if(dll AND EXISTS "${MINGW_BIN_DIR}/${dll}")
            add_custom_command(TARGET Task3 POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${MINGW_BIN_DIR}/${dll}"
                        "$<TARGET_FILE_DIR:Task3>/${dll}"
                COMMENT "Copying ${dll} from ${MINGW_BIN_DIR}"
            )
        endif()
    endforeach()
endif()

#компиляторные оптимизации
option(ENABLE_AGGRESSIVE_OPTIMIZATIONS "Enable aggressive compiler optimizations" OFF)

if(ENABLE_AGGRESSIVE_OPTIMIZATIONS)
    if(MSVC)
        target_compile_options(Task3 PRIVATE
            /O2
            /arch:AVX2
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(Task3 PRIVATE
            -O3
            -march=native
            -ffast-math
            -funroll-loops
            -DNDEBUG
        )
    endif()
else()
    if(MSVC)
        target_compile_options(Task3 PRIVATE /O2)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(Task3 PRIVATE -O2 -DNDEBUG)
    endif()
endif()

include(GNUInstallDirs)

install(TARGETS Task3
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
