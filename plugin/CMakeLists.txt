cmake_minimum_required(VERSION 3.16)
project(obs-ptz-plugin LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
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

elseif (WIN32)
	find_package(CURL REQUIRED)
	find_path(OBS_INCLUDE_DIR "obs-module.h" REQUIRED)
	find_library(OBS_LIBRARY NAMES obs libobs REQUIRED)

	set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIR})
	set(LIBOBS_LIBRARIES ${OBS_LIBRARY})
	set(CURL_LIBRARIES CURL::libcurl)

else ()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(OBS REQUIRED libobs)
	pkg_check_modules(CURL REQUIRED libcurl)

	set(LIBOBS_INCLUDE_DIRS ${OBS_INCLUDE_DIRS})
	set(LIBOBS_LIBRARIES ${OBS_LIBRARIES})
	set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
	set(CURL_LIBRARIES ${CURL_LIBRARIES})
endif ()

#
# Optional Qt dock: needs Qt Widgets and obs-frontend-api (same major OBS/Qt as your install).
# Enable: cmake -B build -DWITH_PTZ_DOCK=ON  (and set CMAKE_PREFIX_PATH / OBS paths if needed)
#
option(WITH_PTZ_DOCK "Build colored PTZ dock (Qt + obs-frontend-api)" OFF)

set(PTZ_DOCK_ACTIVE OFF)
if (WITH_PTZ_DOCK)
	find_package(Qt6 QUIET COMPONENTS Core Widgets)
	if (NOT Qt6_FOUND)
		find_package(Qt5 QUIET COMPONENTS Core Widgets)
	endif ()

	find_path(OBS_FRONTEND_INCLUDE_DIR
		NAMES obs-frontend-api.h
		HINTS
		${LIBOBS_INCLUDE_DIRS}
		ENV OBS_INCLUDE_DIR
		PATH_SUFFIXES
		obs
		UI/obs-frontend-api
	)

	find_library(OBS_FRONTEND_LIBRARY
		NAMES obs-frontend-api libobs-frontend-api
		HINTS
		ENV OBS_FRONTEND_LIB_DIR
		"$ENV{ProgramFiles}/obs-studio/bin/64bit"
	)

	if ((Qt6_FOUND OR Qt5_FOUND) AND OBS_FRONTEND_INCLUDE_DIR AND OBS_FRONTEND_LIBRARY)
		set(PTZ_DOCK_ACTIVE ON)
	else ()
		message(WARNING "WITH_PTZ_DOCK=ON but Qt or obs-frontend-api not found; building without dock.")
	endif ()
endif ()

add_library(${PROJECT_NAME} MODULE
	my-plugin.c
	ptz-core.c
)

if (PTZ_DOCK_ACTIVE)
	target_sources(${PROJECT_NAME} PRIVATE ptz-dock.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_PTZ_DOCK)
	target_include_directories(${PROJECT_NAME} PRIVATE ${OBS_FRONTEND_INCLUDE_DIR})

	if (Qt6_FOUND)
		target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core)
	else ()
		target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Core)
	endif ()

	target_link_libraries(${PROJECT_NAME} PRIVATE ${OBS_FRONTEND_LIBRARY})
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBOBS_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBOBS_LIBRARIES} ${CURL_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
