cmake_minimum_required(VERSION 3.16)
project(R2000)

set(CMAKE_SYSTEM_NAME Linux)
add_compile_options(-Wall -Wextra)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(DEBUG_FLAGS "-g -ggdb -O0 -pthread -pedantic")
set(RELEASE_FLAGS "-O3 -pthread -pedantic")

include_directories(include)
# Threads       ############################################################
find_package(Threads REQUIRED)
# Boost Package ############################################################
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.64.0 COMPONENTS REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
############################################################################

add_executable(DeviceEventWatch include
        src/R2000.cpp
        src/StatusWatcher.cpp
        examples/WatchDeviceEvents.cpp)
target_link_libraries(DeviceEventWatch
        PRIVATE ${CMAKE_DL_LIBS}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${Boost_LIBRARIES} Threads::Threads)

add_executable(DeviceStatusWatch include
        src/R2000.cpp
        src/StatusWatcher.cpp
        examples/WatchDeviceStatus.cpp)
target_link_libraries(DeviceStatusWatch
        PRIVATE ${CMAKE_DL_LIBS}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${Boost_LIBRARIES} Threads::Threads)

add_executable(AcquireScan include
        src/DataLink/DataLinkBuilder.cpp
        src/DataLink/DataLink.cpp
        src/DataLink/TCPLink.cpp
        src/DataLink/UDPLink.cpp
        src/R2000.cpp
        examples/AcquireScan.cpp)
target_link_libraries(AcquireScan
        PRIVATE ${CMAKE_DL_LIBS}
        ${Boost_LIBRARIES}
        ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${PCL_LIBRARIES}
        Threads::Threads)