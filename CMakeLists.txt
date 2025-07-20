cmake_minimum_required(VERSION 3.16)
project(OrderBookSimulator VERSION 1.0 LANGUAGES CXX)

# Require C++17 or later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include header files
include_directories(include)

# Gather all source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Define the executable
add_executable(OrderBookSimulator ${SOURCES})

# Use threads
find_package(Threads REQUIRED)
target_link_libraries(OrderBookSimulator PRIVATE Threads::Threads)

# Optional: warnings and debug symbols
target_compile_options(OrderBookSimulator PRIVATE -Wall -Wextra -O2)
