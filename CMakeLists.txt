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

# PCL Package ##############################################################
find_package(PCL 1.8 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})
############################################################################

# JSONCPP Package ##########################################################
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})
############################################################################

add_executable(CommandsTest include
        src/Backtrace.cpp
        src/R2000/R2000.cpp
        CommandsTest.cpp
        include/R2000/StatusWatcher.hpp)
target_link_libraries(CommandsTest ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES})

add_executable(DataLinkTest include
        src/R2000/DataLink/DataLink.cpp
        src/R2000/DataLink/TCPLink.cpp
        src/R2000/DataLink/UDPLink.cpp
        src/R2000/R2000.cpp
        src/R2000/StatusWatcher.cpp
        src/Backtrace.cpp
        DataLinkTest.cpp)
target_link_libraries(DataLinkTest ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES}  ${PCL_LIBRARIES} ${JSONCPP_LIBRARIES})