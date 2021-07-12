cmake_minimum_required(VERSION 3.19)
project(R2000)

set(CMAKE_SYSTEM_NAME Linux)

add_compile_options(-Wall -Wextra)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# added -fopenmp
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

set(DEBUG_FLAGS "-g -ggdb -O0 -pthread -pedantic")
set(RELEASE_FLAGS "-O3 -pthread -pedantic")

include_directories(include)

# Boost Package ############################################################
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.58.0 COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
############################################################################

# JSONCPP Package ##########################################################
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})
############################################################################

add_executable(R2000 include
        DataLink/DataLink.cpp
        DataLink/TCPLink.cpp
        DataLink/UDPLink.cpp
        DeviceHandle.cpp
        HttpController.cpp
        main.cpp)
target_link_libraries(R2000 ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES})