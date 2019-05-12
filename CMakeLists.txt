cmake_minimum_required(VERSION 3.14)
project(ThreadPool C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
add_executable(ThreadPool src/threadpool.c src/threadpool.h src/osqueue.c src/osqueue.h tests/sanity_test.c)

TARGET_LINK_LIBRARIES(ThreadPool pthread)
