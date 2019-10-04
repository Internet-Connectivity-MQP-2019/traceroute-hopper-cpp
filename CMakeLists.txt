cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME traceroute_hopper)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -m64")

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)

# Source code 
add_library(${PROJECT_NAME}_lib ${SRC_FILES})
set_target_properties(${PROJECT_NAME}_lib PROPERTIES LINKER_LANGUAGE CXX)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)


# LIBRARIES

# json
find_package(nlohmann_json 3.2.0 REQUIRED)
target_link_libraries(traceroute_hopper PRIVATE nlohmann_json::nlohmann_json)

# postgres
find_package(PostgreSQL REQUIRED)
target_link_libraries(traceroute_hopper PRIVATE ${PostgreSQL_LIBRARIES})
target_include_directories(traceroute_hopper PRIVATE ${PostgreSQL_INCLUDE_DIRS})

