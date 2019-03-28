cmake_minimum_required(VERSION 3.0.2)
project(sdk2_agents)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

add_subdirectory("IFLYOSInterface")
add_subdirectory("IFLYOSUmbrella")
add_subdirectory("MicAgent")