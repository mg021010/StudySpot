cmake_minimum_required(VERSION 3.10)
project(StudySpot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    node/interfaces
    node/drivers
    node/services
    bridge
)

# Detect System
if(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    add_definitions(-D__linux__)
endif()

# Find libraries ONLY on Linux
if(IS_LINUX)
    find_library(PIGPIO_LIB pigpio)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED alsa)
    pkg_check_modules(BLUEZ REQUIRED bluez)
    
    set(EXTRA_LIBS ${PIGPIO_LIB} ${ALSA_LIBRARIES} ${BLUEZ_LIBRARIES})
else()
    message(STATUS "Running on Non-Linux system. Hardware drivers will operate in MOCK mode.")
endif()

# Main Executable
add_executable(study_spot_node node/main.cpp)

# Link libraries
target_link_libraries(study_spot_node 
    ${EXTRA_LIBS}
    paho-mqttpp3
    paho-mqtt3as
    pthread
)
