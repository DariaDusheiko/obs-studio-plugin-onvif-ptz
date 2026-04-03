cmake_minimum_required(VERSION 3.10)
project(obs-ptz-plugin C)

set(CMAKE_C_STANDARD 99)

if(APPLE)
    # macOS: Homebrew paths vary by architecture
    find_path(OBS_INCLUDE_DIR "obs-module.h"
        PATHS
            /usr/local/include
            /opt/homebrew/include
        REQUIRED)

    find_library(OBS_LIBRARY NAMES obs
        PATHS
            /usr/local/lib
            /opt/homebrew/lib
        REQUIRED)

    find_package(CURL REQUIRED)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARY})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})

elseif(WIN32)
    # Windows: vcpkg
    find_package(CURL REQUIRED)
    # В новых версиях vcpkg пакет называется libobs
    find_path(OBS_INCLUDE_DIR "obs-module.h" REQUIRED)
    find_library(OBS_LIBRARY NAMES obs libobs REQUIRED)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARY})
    set(CURL_LIBRARIES CURL::libcurl)

else()
    # Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OBS REQUIRED libobs)
    pkg_check_modules(CURL REQUIRED libcurl)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIRS})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARIES})
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})
endif()

add_library(${PROJECT_NAME} MODULE my-plugin.c)

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBOBS_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBOBS_LIBRARIES} ${CURL_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")