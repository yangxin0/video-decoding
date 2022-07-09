cmake_minimum_required(VERSION 3.10.0)
project(video-decoding)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
find_package(PkgConfig REQUIRED)
pkg_search_module(AVFORMAT REQUIRED libavformat)
pkg_search_module(AVCODEC REQUIRED libavcodec)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${AVFORMAT_INCLUDE_DIRS})
include_directories(${AVCODEC_INCLUDE_DIRS})

link_directories(${AVFORMAT_LIBRARY_DIRS})
link_directories(${AVCODEC_LIBRARY_DIRS})

add_executable(${CMAKE_PROJECT_NAME} ${SRC})
target_link_libraries(${CMAKE_PROJECT_NAME} ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES})

