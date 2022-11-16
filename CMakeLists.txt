cmake_minimum_required(VERSION 3.16)
project(R2000 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEBUG_FLAGS "-g -ggdb -O0 -pthread -pedantic")
set(RELEASE_FLAGS "-O3 -pthread -pedantic")

add_compile_options(-Wall -Wextra)

# Thread Package ###########################################################
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Boost Package ############################################################
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.64.0 COMPONENTS REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})


############################################################################
#                       Examples Targets Definition                        #
############################################################################

include_directories(include)
set(PROJECT_SHARED_SOURCES include src/R2000.cpp src/StatusWatcher.cpp)
set(PROJECT_DATA_LINK_SOURCES src/DataLink/DataLinkBuilder.cpp
        src/DataLink/DataLink.cpp
        src/DataLink/TCPLink.cpp
        src/DataLink/UDPLink.cpp)
set(PROJECT_LIBRARIES ${CMAKE_DL_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARIES}
        ${Boost_LIBRARIES} Threads::Threads)

# Device Event ############################################################
set(TARGET_DEVICE_EVENT_NAME DeviceEventWatch)
set(TARGET_DEVICE_EVENT_MAIN examples/WatchDeviceEvents.cpp)
add_executable(${TARGET_DEVICE_EVENT_NAME} include
        ${PROJECT_SHARED_SOURCES}
        ${TARGET_DEVICE_EVENT_MAIN})
target_link_libraries(${TARGET_DEVICE_EVENT_NAME}
        PRIVATE ${PROJECT_LIBRARIES})
list(APPEND TARGETS ${TARGET_DEVICE_EVENT_NAME})
# Device Status ############################################################
set(TARGET_DEVICE_STATUS_NAME DeviceStatusWatch)
set(TARGET_DEVICE_STATUS_MAIN examples/WatchDeviceStatus.cpp)
add_executable(${TARGET_DEVICE_STATUS_NAME} include
        ${PROJECT_SHARED_SOURCES}
        ${TARGET_DEVICE_STATUS_MAIN})
target_link_libraries(${TARGET_DEVICE_STATUS_NAME}
        PRIVATE ${PROJECT_LIBRARIES})
list(APPEND TARGETS ${TARGET_DEVICE_STATUS_NAME})
# Scan Acquisition ###########################################################
set(TARGET_ACQUIRE_SCAN_NAME AcquireScan)
set(TARGET_ACQUIRE_SCAN_MAIN examples/AcquireScan.cpp)
add_executable(${TARGET_ACQUIRE_SCAN_NAME} include
        ${PROJECT_DATA_LINK_SOURCES}
        ${PROJECT_SHARED_SOURCES}
        ${TARGET_ACQUIRE_SCAN_MAIN})
target_link_libraries(${TARGET_ACQUIRE_SCAN_NAME}
        PRIVATE ${PROJECT_LIBRARIES})
list(APPEND TARGETS ${TARGET_ACQUIRE_SCAN_NAME})

message(STATUS "List of target examples to build : ${TARGETS}")