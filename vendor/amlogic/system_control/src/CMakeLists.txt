cmake_minimum_required(VERSION 3.0)
project(system_control)
set(PROJECT_SYSROOT $ENV{STAGING_DIR})

set(CMAKE_BUILD_TYPE Release)
#include_directories(
#    ${PROJECT_SYSROOT}/usr/include
#)

#link_directories(
#    ${PROJECT_SYSROOT}/usr/lib
#)

if(AML_OSD_USE_DRM)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAML_OSD_USE_DRM")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

set(SYSTEM_CONTROL_SOURCES
     DisplayMode.cpp
     FormatColorDepth.cpp
     FrameRateAutoAdaption.cpp
     HDCPTxAuth.cpp
     SysTokenizer.cpp
     SysWrite.cpp
     SystemControl.cpp
     TxUEventCallback.cpp
     UEventObserver.cpp
     main_systemcontrol.cpp
     ubootenv.c
)

set(DISP_UTIL_SOURCES
    main_client.cpp
)

add_executable(systemcontrol ${SYSTEM_CONTROL_SOURCES})
add_executable(display_util ${DISP_UTIL_SOURCES})

target_link_libraries(systemcontrol pthread z)

install(TARGETS systemcontrol DESTINATION bin)
install(TARGETS display_util DESTINATION bin)
