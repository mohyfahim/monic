cmake_minimum_required(VERSION 3.10)
project(monic C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_library(sqlite3 STATIC sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(monic_log STATIC log.c)
target_include_directories(monic_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(monic_log PUBLIC LOG_USE_COLOR)

add_library(monic_dns STATIC dns.c)
target_include_directories(monic_dns PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(monic_ping STATIC ping.c)
target_include_directories(monic_ping PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monic_ping PRIVATE monic_dns)

add_library(monic_tcp STATIC tcp.c)
target_include_directories(monic_tcp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monic_tcp PRIVATE monic_dns)

add_library(monic_netlink STATIC netlink.cpp)
target_include_directories(monic_netlink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monic_netlink PRIVATE monic_log)

add_executable(monic main.cpp)
target_include_directories(monic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monic PRIVATE sqlite3 monic_log monic_netlink monic_dns monic_tcp monic_ping)
add_custom_command(TARGET monic POST_BUILD COMMAND strip -s monic)
