cmake_minimum_required(VERSION 3.15)
project(traceroute_hopper)

set(CMAKE_CXX_STANDARD 17)

add_executable(traceroute_hopper main.cpp cmdline.h cmdline.c)

find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(PostgreSQL REQUIRED)

target_link_libraries(traceroute_hopper PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(traceroute_hopper PRIVATE ${PostgreSQL_LIBRARIES})
target_include_directories(traceroute_hopper PRIVATE ${PostgreSQL_INCLUDE_DIRS})
