cmake_minimum_required(VERSION 3.5)
project(redis_fence)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES redis-fence.c)

add_executable(redis_fence ${SOURCE_FILES})