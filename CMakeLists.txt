cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME traceroute_hopper)
project(${PROJECT_NAME})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -m64")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")

# Source code 
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/main.cpp ${PROJECT_SOURCE_DIR}/src/cmdline.c src/caida.cpp src/caida.hpp src/atlas.cpp src/atlas.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)


# LIBRARIES

# json
#find_package(nlohmann_json 3.2.0 REQUIRED)
#target_link_libraries(traceroute_hopper PRIVATE nlohmann_json::nlohmann_json)
find_package(RapidJSON)

# postgres
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)
target_link_libraries(traceroute_hopper PRIVATE ${PQXX_LIB} ${PQ_LIB})
target_include_directories(traceroute_hopper PRIVATE ${PQXX_INCLUDE_DIRS})

