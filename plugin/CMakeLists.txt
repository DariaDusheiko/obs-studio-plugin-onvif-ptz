cmake_minimum_required(VERSION 3.10)
project(obs-ptz-plugin C)

set(CMAKE_C_STANDARD 99)

# --- Поиск зависимостей ---

if(APPLE)
    # macOS: Обычно заголовочные файлы лежат в /usr/local/include после brew install obs
    find_path(OBS_INCLUDE_DIR "obs-module.h" PATHS "/usr/local/include" "/opt/homebrew/include")
    find_library(OBS_LIBRARY NAMES obs PATHS "/usr/local/lib" "/opt/homebrew/lib")
    find_package(CURL REQUIRED)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARY})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})

elseif(WIN32)
    # Windows: Используем vcpkg только для curl, а OBS подхватим через переменную окружения
    find_package(CURL REQUIRED)

    # Пытаемся найти libobs (должна быть в путях vcpkg или передана через -DOBS_PATH)
    find_path(OBS_INCLUDE_DIR "obs-module.h")
    find_library(OBS_LIBRARY "obs")

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARY})
    set(CURL_LIBRARIES CURL::libcurl)

else()
    # Linux: Самый надежный способ — pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OBS REQUIRED libobs)
    pkg_check_modules(CURL REQUIRED libcurl)

    set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIRS})
    set(LIBOBS_LIBRARIES ${OBS_LIBRARIES})
    set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})
endif()

# --- Сборка ---

add_library(${PROJECT_NAME} MODULE my-plugin.c)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIBOBS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBOBS_LIBRARIES}
    ${CURL_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "my-plugin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
    )
endif()