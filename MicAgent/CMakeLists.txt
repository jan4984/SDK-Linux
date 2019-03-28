cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(MicAgent)

find_package(Threads ${THREADS_PACKAGE_CONFIG})
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funwind-tables -fexceptions")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funwind-tables -fexceptions")

add_executable(MicAgent
        MicAgent.cpp
        tdmin_interface.c
        NetworkMonitor.cpp
        NetworkSetter.cpp
        PCMPlayer.cpp
        Recorder.cpp
        syskey.cpp
        )

find_package(alsa)
#find_package(CURL)
find_package(portaudio)

target_include_directories(MicAgent PRIVATE
        "${IFLYOSInterface_SOURCE_DIR}/include"
        #"${CURL_INCLUDE_DIRS}"
        "${PA_INCLUDE_DIRS}"
        "${ALSA_INCLUDE_DIRS}")

target_link_libraries(MicAgent  IFLYOSInterface
        ${ALSA_LDFLAGS}  ${PA_LDFLAGS}
        #${CURL_LIBRARIES}
        #"-lfec -lliquid -lquiet -ljansson -lfftw3f"
        "${CMAKE_CURRENT_SOURCE_DIR}/libsnowboy-detect.a"
        "-L/rpxc/sysroot/usr/lib"
        "-L/rpxc/sysroot/usr/lib/arm-linux-gnueabihf"
        "-L/rpxc/sysroot/usr/lib/atlas-base"
        "-L/rpxc/sysroot/usr/lib/atlas-base/atlas/"
        "-latlas -lblas -lcblas -lgfortran"
        ${CMAKE_THREAD_LIBS_INIT}
        )

install(TARGETS MicAgent RUNTIME DESTINATION "/usr/bin")
