cmake_minimum_required(VERSION 3.16)
project(R2000)

set(CMAKE_SYSTEM_NAME Linux)

add_compile_options(-Wall -Wextra)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEBUG_FLAGS "-g -ggdb -O0 -pthread -pedantic")
set(RELEASE_FLAGS "-O3 -pthread -pedantic")

include_directories(include)

# Boost Package ############################################################
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.68.0 COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
############################################################################

# PCL Package ##############################################################
find_package(PCL 1.8 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})
############################################################################

add_executable(TcpAcquisition include
        src/R2000/DataLink/DataLinkBuilder.cpp
        src/R2000/DataLink/DataLink.cpp
        src/R2000/DataLink/TCPLink.cpp
        src/R2000/DataLink/UDPLink.cpp
        src/R2000/R2000.cpp
        Applications/TcpAcquisition.cpp)
target_link_libraries(TcpAcquisition
        ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${PCL_LIBRARIES})

add_executable(UdpAcquisition include
        src/R2000/DataLink/DataLinkBuilder.cpp
        src/R2000/DataLink/DataLink.cpp
        src/R2000/DataLink/TCPLink.cpp
        src/R2000/DataLink/UDPLink.cpp
        src/R2000/R2000.cpp
        Applications/UdpAcquisition.cpp)
target_link_libraries(UdpAcquisition
        ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${PCL_LIBRARIES})

add_executable(DeviceWatcher include
        src/R2000/R2000.cpp
        src/R2000/StatusWatcher.cpp
        Applications/DeviceWatcher.cpp)
target_link_libraries(DeviceWatcher
        ${CMAKE_DL_LIBS}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${Boost_LIBRARIES})

add_executable(DeviceEvent include
        src/R2000/R2000.cpp
        src/R2000/StatusWatcher.cpp
        Applications/DeviceEvent.cpp)
target_link_libraries(DeviceEvent
        ${CMAKE_DL_LIBS}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${Boost_LIBRARIES})