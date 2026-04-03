cmake_minimum_required(VERSION 3.10)
project(my-plugin C)

set(CMAKE_C_STANDARD 99)

if(WIN32)
    # Windows: используем vcpkg или手工安装的依赖
    # Установите зависимости через vcpkg: vcpkg install libcurl:x64-windows libobs:x64-windows
    
    if(NOT DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_ROOT "$ENV{VCPKG_ROOT}")
    endif()
    
    # Пути для vcpkg
    set(VCPKG_INSTALL_PREFIX "C:/vcpkg/installed/x64-windows")
    
    # libobs
    find_path(LIBOBS_INCLUDE_DIR "obs-module.h" 
        PATHS 
            "${VCPKG_INSTALL_PREFIX}/include"
            "$ENV{OBS_PATH}/obs-studio/build/libobs"
            "C:/obs-studio/build/libobs"
        NO_DEFAULT_PATH)
    find_library(LIBOBS_LIBRARY "obs" 
        PATHS 
            "${VCPKG_INSTALL_PREFIX}/lib"
            "$ENV{OBS_PATH}/obs-studio/build/libobs"
            "C:/obs-studio/build/libobs"
        NO_DEFAULT_PATH)
    
    # libcurl
    find_path(CURL_INCLUDE_DIR "curl/curl.h" 
        PATHS 
            "${VCPKG_INSTALL_PREFIX}/include"
            "C:/vcpkg/installed/x64-windows/include"
        NO_DEFAULT_PATH)
    find_library(CURL_LIBRARY "libcurl" 
        PATHS 
            "${VCPKG_INSTALL_PREFIX}/lib"
            "C:/vcpkg/installed/x64-windows/lib"
        NO_DEFAULT_PATH)
    
    if(NOT CURL_LIBRARY)
        find_library(CURL_LIBRARY "curl" 
            PATHS 
                "${VCPKG_INSTALL_PREFIX}/lib"
            NO_DEFAULT_PATH)
    endif()
    
    set(LIBOBS_INCLUDE_DIRS "${LIBOBS_INCLUDE_DIR}")
    set(LIBOBS_LIBRARIES "${LIBOBS_LIBRARY}")
    set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIR}")
    set(CURL_LIBRARIES "${CURL_LIBRARY}")
    
    message(STATUS "LIBOBS: ${LIBOBS_INCLUDE_DIRS} / ${LIBOBS_LIBRARIES}")
    message(STATUS "CURL: ${CURL_INCLUDE_DIRS} / ${CURL_LIBRARIES}")
    
else()
    # Linux/macOS
    find_package(PkgConfig REQUIRED)
    find_package(libobs REQUIRED)
    pkg_check_modules(CURL REQUIRED libcurl)
    
    set(LIBOBS_INCLUDE_DIRS "${LIBOBS_INCLUDE_DIRS}")
    set(LIBOBS_LIBRARIES "${LIBOBS_LIBRARIES}")
    set(CURL_INCLUDE_DIRS "${CURL_INCLUDE_DIRS}")
    set(CURL_LIBRARIES "${CURL_LIBRARIES}")
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

set_target_properties(my-plugin PROPERTIES
    PREFIX ""
)

if(WIN32)
    # Для Windows выходной файл должен быть в правильной директории
    set_target_properties(my-plugin PROPERTIES
        OUTPUT_NAME "obs-plugins\\64bit\\my-plugin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/obs-plugins/64bit"
    )
endif()
