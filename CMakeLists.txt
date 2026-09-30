cmake_minimum_required(VERSION 3.16)
project(xfce4-sysusage-plugin VERSION 2.14 LANGUAGES C)
set(GETTEXT_PACKAGE "${PROJECT_NAME}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SYSUSAGE_ENABLE_SANITIZERS "Build with address and undefined behavior sanitizers" OFF)
set(SYSUSAGE_CLANG_TIDY_CHECKS "-*,clang-analyzer-core.*,clang-analyzer-deadcode.*,clang-analyzer-security.*,clang-analyzer-unix.*"
    CACHE STRING "clang-tidy checks used by the clang-tidy target")


include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(XFCE4PANEL REQUIRED libxfce4panel-2.0)
pkg_check_modules(XFCE4UTIL REQUIRED libxfce4util-1.0)


pkg_check_modules(GLIB2 REQUIRED glib-2.0)

enable_testing()

add_executable(sysusage-logic-tests
    tests-sysusage-logic.c
    src/sysusage-logic.c
)

target_include_directories(sysusage-logic-tests PRIVATE
    ${GLIB2_INCLUDE_DIRS}
)

target_compile_options(sysusage-logic-tests PRIVATE
    -Wall
    -Wextra
    ${GLIB2_CFLAGS_OTHER}
)

target_link_libraries(sysusage-logic-tests PRIVATE
    ${GLIB2_LIBRARIES}
)

add_test(NAME sysusage-logic-tests COMMAND sysusage-logic-tests)

set(SYSUSAGE_SOURCES
    src/sysusage-plugin.c
    src/sysusage-render.c
    src/sysusage-metrics.c
    src/sysusage-config.c
    src/sysusage-properties.c
    src/sysusage-logic.c
)

add_library(sysusage MODULE ${SYSUSAGE_SOURCES})

target_include_directories(sysusage PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${XFCE4PANEL_INCLUDE_DIRS}
    ${XFCE4UTIL_INCLUDE_DIRS}
)

target_compile_definitions(sysusage PRIVATE
    SYSUSAGE_PLUGIN_VERSION="${PROJECT_VERSION}"
    GETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
    LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
)

target_compile_options(sysusage PRIVATE
    -Wall
    -Wextra
    ${GTK3_CFLAGS_OTHER}
    ${XFCE4PANEL_CFLAGS_OTHER}
    ${XFCE4UTIL_CFLAGS_OTHER}
)

target_link_libraries(sysusage PRIVATE
    ${GTK3_LIBRARIES}
    ${XFCE4PANEL_LIBRARIES}
    ${XFCE4UTIL_LIBRARIES}
)

if(SYSUSAGE_ENABLE_SANITIZERS)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(sysusage PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(sysusage PRIVATE -fsanitize=address,undefined)
    else()
        message(WARNING "SYSUSAGE_ENABLE_SANITIZERS is only supported with GCC or Clang")
    endif()
endif()

set(SYSUSAGE_STATIC_ANALYSIS_TARGETS)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY_EXE}
            -p ${CMAKE_BINARY_DIR}
            --checks=${SYSUSAGE_CLANG_TIDY_CHECKS}
            ${SYSUSAGE_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy static analysis"
        VERBATIM
    )
    list(APPEND SYSUSAGE_STATIC_ANALYSIS_TARGETS clang-tidy)
else()
    message(STATUS "clang-tidy not found; clang-tidy target will not be available")
endif()

find_program(CPPCHECK_EXE NAMES cppcheck)
if(CPPCHECK_EXE)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXE}
            --enable=warning,style,performance,portability
            --error-exitcode=1
            --inline-suppr
            --project=${CMAKE_BINARY_DIR}/compile_commands.json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis"
        VERBATIM
    )
    list(APPEND SYSUSAGE_STATIC_ANALYSIS_TARGETS cppcheck)
else()
    message(STATUS "cppcheck not found; cppcheck target will not be available")
endif()

if(SYSUSAGE_STATIC_ANALYSIS_TARGETS)
    add_custom_target(static-analysis DEPENDS ${SYSUSAGE_STATIC_ANALYSIS_TARGETS})
else()
    add_custom_target(static-analysis
        COMMAND ${CMAKE_COMMAND} -E echo "No supported static-analysis tools found; install clang-tidy or cppcheck."
        VERBATIM
    )
endif()
add_custom_target(lint DEPENDS static-analysis)

install(TARGETS sysusage
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/xfce4/panel/plugins
)

configure_file(panel-plugin/sysusage.desktop.in panel-plugin/sysusage.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/panel-plugin/sysusage.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/xfce4/panel/plugins
)

install(FILES xfce4-sysusage-plugin.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
