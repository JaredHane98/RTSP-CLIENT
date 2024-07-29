cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(SERVER_PROGRAM VERSION 1.0.0 LANGUAGES CXX)


add_compile_options(-fPIC)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPENCV_MIN_VERSION "4.4") 


if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()




find_package(X11 REQUIRED)
    if(NOT X11_FOUND)
        message(FETAL_ERROR "FAILED TO FIND X11")
    else()
        message("X11_FOUND")
    endif()

find_package(OpenCV ${OPENCV_MIN_VERSION} REQUIRED)
    if(NOT OpenCV_FOUND)
        message(FATAL_ERROR "OpenCV > 4.4 not found.")
    else()
        message("OPENCV VERSION FOUND: ${OpenCV_VERSION}")
    endif()


find_package(PkgConfig REQUIRED)
pkg_check_modules(gstreamer REQUIRED gstreamer-1.0)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0 >= 1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0 >= 1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0 >= 1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0 >= 1.4)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_executable(${PROJECT_NAME}
                 main.cpp)

include_directories(${PROJECT_SOURCE_DIR}
                    ${GSTREAMER_INCLUDE_DIRS})


link_directories(${GLIB_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME}
                     ${GLIB_LIBRARIES}
                     PkgConfig::gstreamer
                     PkgConfig::gstreamer-sdp
                     PkgConfig::gstreamer-app
                     PkgConfig::gstreamer-video)
