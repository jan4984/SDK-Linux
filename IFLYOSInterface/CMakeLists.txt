cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(IFLYOSInterface LANGUAGES CXX)

add_library(IFLYOSInterface STATIC src/IFLYOSInterface.cpp src/IFLYOSThreadPool.cpp)

if(LOG_MSG)
    add_definitions("-DLOG_MSG")
endif()

find_package(Threads REQUIRED)
target_link_libraries(IFLYOSInterface ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(IFLYOSInterface PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
