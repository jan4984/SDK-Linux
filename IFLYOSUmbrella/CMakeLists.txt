cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(IFLYOSUmbrella)

find_package(Threads ${THREADS_PACKAGE_CONFIG})
add_executable(IFLYOSUmbrella IFLYOSUmbrella.cpp)

target_include_directories(IFLYOSUmbrella PRIVATE
        "${IFLYOSInterface_SOURCE_DIR}/include")

target_link_libraries(IFLYOSUmbrella IFLYOSInterface IFLYOSInterface ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS IFLYOSUmbrella RUNTIME DESTINATION "/usr/bin")

