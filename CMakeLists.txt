cmake_minimum_required(VERSION 3.0)
add_compile_options(-std=c++11)

project(asio_sync)
find_package(Boost REQUIRED system)
find_package(Threads)

include_directories(${Boost_INCLUDE_DIR})

add_executable(client
        client.cpp
        )
target_link_libraries(client
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

