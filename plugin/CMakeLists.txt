cmake_minimum_required(VERSION 3.10)
project(my-plugin C)

set(CMAKE_C_STANDARD 99)

if(WIN32)
    # На Windows полагаемся на vcpkg
    find_path(LIBOBS_INCLUDE_DIR "obs-module.h")
    find_library(LIBOBS_LIBRARY "obs")
    find_package(CURL REQUIRED)

    set(LIBOBS_INCLUDE_DIRS "${LIBOBS_INCLUDE_DIR}")
    set(LIBOBS_LIBRARIES "${LIBOBS_LIBRARY}")
    set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIRS}")
    set(CURL_LIBRARIES "CURL::libcurl")
else()
    # На Linux используем pkg-config (исправляет ошибку "obs-module.h not found")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OBS REQUIRED libobs)
    pkg_check_modules(CURL REQUIRED libcurl)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIRS})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARIES})
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})
endif()

add_library(my-plugin MODULE my-plugin.c)

target_include_directories(my-plugin PRIVATE
    ${LIBOBS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(my-plugin
    ${LIBOBS_LIBRARIES}
    ${CURL_LIBRARIES}
)

set_target_properties(my-plugin PROPERTIES PREFIX "")

if(WIN32)
    set_target_properties(my-plugin PROPERTIES
        OUTPUT_NAME "my-plugin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
    )
endif()