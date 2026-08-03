cmake_minimum_required(VERSION 3.16)

project(M5CardputerZeroCapLoRa1262GPS VERSION 0.1.1 LANGUAGES C CXX ASM)

option(CAP_GPS_USE_SDL "Use LVGL's SDL backend and mock GPS receiver" ON)
option(BUILD_TESTING "Build Cap-LoRa-1262-GPS tests" OFF)
set(CAP_GPS_BIN_NAME "M5CardputerZero-Cap-LoRa-1262-GPS" CACHE STRING "Output executable name")
set(CAP_GPS_DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/dependencies" CACHE PATH "Third-party source directory")

if(CAP_GPS_USE_SDL)
    set(CAP_GPS_DEFAULT_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/dist/sdl")
else()
    set(CAP_GPS_DEFAULT_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/dist/device")
endif()
if(NOT DEFINED CAP_GPS_OUTPUT_DIR)
    set(CAP_GPS_OUTPUT_DIR "${CAP_GPS_DEFAULT_OUTPUT_DIR}" CACHE PATH "Runtime output directory")
endif()

include(CTest)

if(APPLE AND NOT CAP_GPS_USE_SDL)
    message(FATAL_ERROR "macOS builds only support CAP_GPS_USE_SDL=ON.")
endif()
if(NOT CAP_GPS_USE_SDL AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "The GPS UART backend requires Linux.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CAP_GPS_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(CAP_GPS_SRC_DIR "${CAP_GPS_ROOT}/src")
set(CAP_GPS_LVGL_DIR "${CAP_GPS_DEPS_DIR}/lvgl")
set(CAP_GPS_SPDLOG_DIR "${CAP_GPS_DEPS_DIR}/spdlog")
set(CAP_GPS_SMOOTH_UI_DIR "${CAP_GPS_DEPS_DIR}/smooth_ui_toolkit")
set(CAP_GPS_MINMEA_DIR "${CAP_GPS_DEPS_DIR}/minmea")
set(CAP_GPS_SYSTEM_LIBS ${CMAKE_DL_LIBS})

if(UNIX AND NOT APPLE)
    list(APPEND CAP_GPS_SYSTEM_LIBS m)
endif()
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
endif()

foreach(required_dir IN LISTS CAP_GPS_LVGL_DIR CAP_GPS_SPDLOG_DIR CAP_GPS_SMOOTH_UI_DIR)
    if(NOT EXISTS "${required_dir}/CMakeLists.txt")
        message(FATAL_ERROR "Missing dependency: ${required_dir}. Run ./bootstrap.sh first or set CAP_GPS_DEPS_DIR.")
    endif()
endforeach()
if(NOT EXISTS "${CAP_GPS_MINMEA_DIR}/minmea.c" OR NOT EXISTS "${CAP_GPS_MINMEA_DIR}/minmea.h")
    message(FATAL_ERROR "Missing dependency: ${CAP_GPS_MINMEA_DIR}. Run ./bootstrap.sh first.")
endif()

find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

set(LV_BUILD_CONF_PATH "${CAP_GPS_ROOT}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SMOOTH_UI_TOOLKIT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SMOOTH_UI_TOOLKIT_BUILD_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory("${CAP_GPS_LVGL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/dependencies/lvgl" EXCLUDE_FROM_ALL)
add_subdirectory("${CAP_GPS_SPDLOG_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/dependencies/spdlog" EXCLUDE_FROM_ALL)
add_subdirectory("${CAP_GPS_SMOOTH_UI_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/dependencies/smooth_ui_toolkit"
                 EXCLUDE_FROM_ALL)

function(cap_gps_add_sdl2_parent_include target imported_target)
    if(NOT TARGET "${imported_target}")
        return()
    endif()
    get_target_property(sdl2_include_dirs "${imported_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT sdl2_include_dirs)
        return()
    endif()
    foreach(sdl2_include_dir IN LISTS sdl2_include_dirs)
        if(sdl2_include_dir MATCHES "/SDL2$")
            get_filename_component(sdl2_parent_include_dir "${sdl2_include_dir}" DIRECTORY)
            target_include_directories("${target}" SYSTEM PRIVATE "${sdl2_parent_include_dir}")
        endif()
    endforeach()
endfunction()

set(CAP_GPS_LVGL_POSIX_PATH "./")
if(NOT CAP_GPS_USE_SDL)
    set(CAP_GPS_LVGL_POSIX_PATH "/usr/share/Cap-LoRa-1262-GPS/")
endif()
target_compile_definitions(lvgl PUBLIC
    CAP_GPS_LVGL_USE_SDL=$<IF:$<BOOL:${CAP_GPS_USE_SDL}>,1,0>
    CAP_GPS_LVGL_FS_POSIX_PATH="${CAP_GPS_LVGL_POSIX_PATH}"
)

if(CAP_GPS_USE_SDL)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
    endif()
    if(SDL2_FOUND)
        target_link_libraries(lvgl PRIVATE PkgConfig::SDL2)
        cap_gps_add_sdl2_parent_include(lvgl PkgConfig::SDL2)
        set(CAP_GPS_SDL_LIBS PkgConfig::SDL2)
    else()
        find_package(SDL2 CONFIG QUIET)
        if(TARGET SDL2::SDL2)
            target_link_libraries(lvgl PRIVATE SDL2::SDL2)
            cap_gps_add_sdl2_parent_include(lvgl SDL2::SDL2)
            set(CAP_GPS_SDL_LIBS SDL2::SDL2)
        else()
            find_package(SDL2 REQUIRED)
            target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
            foreach(sdl2_include_dir IN LISTS SDL2_INCLUDE_DIRS)
                if(sdl2_include_dir MATCHES "/SDL2$")
                    get_filename_component(sdl2_parent_include_dir "${sdl2_include_dir}" DIRECTORY)
                    target_include_directories(lvgl SYSTEM PRIVATE "${sdl2_parent_include_dir}")
                endif()
            endforeach()
            set(CAP_GPS_SDL_LIBS ${SDL2_LIBRARIES})
        endif()
    endif()
endif()

add_library(cap_gps_minmea STATIC "${CAP_GPS_MINMEA_DIR}/minmea.c")
target_include_directories(cap_gps_minmea SYSTEM PUBLIC "${CAP_GPS_MINMEA_DIR}")
target_compile_definitions(cap_gps_minmea PUBLIC MINMEA_MAX_SENTENCE_LENGTH=128)

file(GLOB CAP_GPS_APP_CPP CONFIGURE_DEPENDS
    "${CAP_GPS_SRC_DIR}/main.cpp"
    "${CAP_GPS_SRC_DIR}/core/*.cpp"
    "${CAP_GPS_SRC_DIR}/input/*.cpp"
    "${CAP_GPS_SRC_DIR}/models/*.cpp"
    "${CAP_GPS_SRC_DIR}/view_models/*.cpp"
    "${CAP_GPS_SRC_DIR}/views/*.cpp"
)
set(CAP_GPS_HAL_CPP "${CAP_GPS_SRC_DIR}/hal/gps_lvgl_hal.cpp")
set(CAP_GPS_DOMAIN_CPP
    "${CAP_GPS_SRC_DIR}/gps/gps_backend.cpp"
    "${CAP_GPS_SRC_DIR}/gps/gps_types.cpp"
    "${CAP_GPS_SRC_DIR}/gps/nmea_decoder.cpp"
)

if(CAP_GPS_USE_SDL)
    list(APPEND CAP_GPS_DOMAIN_CPP "${CAP_GPS_SRC_DIR}/gps/mock_gps_backend.cpp")
else()
    list(APPEND CAP_GPS_DOMAIN_CPP "${CAP_GPS_SRC_DIR}/gps/linux_serial_backend.cpp")
    list(APPEND CAP_GPS_HAL_CPP "${CAP_GPS_SRC_DIR}/hal/ext5v_power.cpp")
endif()

add_executable(${CAP_GPS_BIN_NAME} ${CAP_GPS_APP_CPP} ${CAP_GPS_HAL_CPP} ${CAP_GPS_DOMAIN_CPP})
target_include_directories(${CAP_GPS_BIN_NAME} PRIVATE "${CAP_GPS_SRC_DIR}")
target_compile_definitions(${CAP_GPS_BIN_NAME} PRIVATE
    CAP_GPS_USE_MOCK_BACKEND=$<IF:$<BOOL:${CAP_GPS_USE_SDL}>,1,0>
    CAP_GPS_USE_LINUX_BACKEND=$<IF:$<BOOL:${CAP_GPS_USE_SDL}>,0,1>
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${CAP_GPS_BIN_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(${CAP_GPS_BIN_NAME} PRIVATE
    cap_gps_minmea
    lvgl::lvgl
    smooth_ui_toolkit
    spdlog::spdlog
    Threads::Threads
    ${CAP_GPS_SDL_LIBS}
    ${CAP_GPS_SYSTEM_LIBS}
)
set_target_properties(${CAP_GPS_BIN_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CAP_GPS_OUTPUT_DIR}")

if(BUILD_TESTING)
    add_executable(cap_gps_nmea_tests
        tests/nmea_decoder_test.cpp
        src/gps/gps_types.cpp
        src/gps/nmea_decoder.cpp
    )
    target_include_directories(cap_gps_nmea_tests PRIVATE "${CAP_GPS_SRC_DIR}")
    target_link_libraries(cap_gps_nmea_tests PRIVATE cap_gps_minmea)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(cap_gps_nmea_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME cap_gps_nmea COMMAND cap_gps_nmea_tests)

    add_executable(cap_gps_model_tests
        tests/gps_model_test.cpp
        src/models/gps_model.cpp
        src/view_models/gps_view_model.cpp
        src/gps/gps_backend.cpp
        src/gps/gps_types.cpp
        src/gps/nmea_decoder.cpp
    )
    target_include_directories(cap_gps_model_tests PRIVATE "${CAP_GPS_SRC_DIR}")
    target_compile_definitions(cap_gps_model_tests PRIVATE CAP_GPS_USE_MOCK_BACKEND=0 CAP_GPS_USE_LINUX_BACKEND=0)
    target_link_libraries(cap_gps_model_tests PRIVATE cap_gps_minmea smooth_ui_toolkit spdlog::spdlog Threads::Threads)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(cap_gps_model_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME cap_gps_model COMMAND cap_gps_model_tests)
endif()
