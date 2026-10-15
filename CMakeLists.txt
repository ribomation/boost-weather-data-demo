cmake_minimum_required(VERSION 3.28)
project(boost_weather_data_demo
        LANGUAGES CXX
        VERSION 1.0
        DESCRIPTION "Boost Weather-Data Demo"
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED yes)
set(CMAKE_CXX_EXTENSIONS no)
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_definitions(APP_NAME="${PROJECT_DESCRIPTION}")
add_compile_definitions(APP_VERSION="${PROJECT_VERSION}")

find_package(Boost 1.92 CONFIG REQUIRED COMPONENTS
        property_tree
        program_options
        beast
        url
        iostreams
        accumulators
)
find_package(OpenSSL REQUIRED)
#find_package(Threads REQUIRED)

find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)
enable_testing()

add_subdirectory(version-1)
add_subdirectory(version-2)
add_subdirectory(version-3)
add_subdirectory(version-4)
