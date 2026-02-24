cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(rpi-seism-reader VERSION 1.0)

# Specify the C++ standard (C++17 or C++20 is recommended)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the executable and where the source files are
add_executable(rpi-seism-reader 
    src/main.cpp 
    src/ads1256.cpp
)

# Tell CMake where to look for header files
target_include_directories(rpi-seism-reader PUBLIC include)

find_library(PIGPIO_LIB pigpio)
target_link_libraries(rpi-seism-reader ${PIGPIO_LIB} pthread)