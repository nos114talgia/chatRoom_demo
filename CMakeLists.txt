cmake_minimum_required(VERSION 3.10.0)
project(chatRoom VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

add_executable(server server.cpp)
add_executable(chatRoom chatRoom.cpp)

target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(chatRoom PRIVATE Threads::Threads)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
