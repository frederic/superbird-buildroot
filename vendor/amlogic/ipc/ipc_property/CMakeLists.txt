cmake_minimum_required(VERSION 3.0.2)
project(ipc-property-service)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_SKIP_BUILD_RPATH TRUE)

set(IPC_PROP_LIB ipc-property)
set(IPC_PROP_LIB_TYPE SHARED)
set(IPC_PROP_SERVICE ipc-property-service)
set(IPC_PROP_CLIENT ipc-property-bin)

#
# ipc property library
#

set(LIB_SOURCES
        src/lib/ipc_property.cc
  )

add_library(${IPC_PROP_LIB} ${IPC_PROP_LIB_TYPE} ${LIB_SOURCES})
target_include_directories(${IPC_PROP_LIB} PUBLIC
  src/lib
  include
  )
target_link_libraries(${IPC_PROP_LIB}
        pthread)

#
# ipc property service
#

set(DAEMON_NAME ${IPC_PROP_SERVICE})
set(DAEMON_MAJOR_VERSION 1)
set(DAEMON_MINOR_VERSION 1)
set(DAEMON_PATCH_VERSION 0)
set(DAEMON_PID_FILE_NAME ${DAEMON_NAME}.pid)
set(DAEMON_LOG_FILE_NAME ${DAEMON_NAME}.log)
set(DAEMON_NO_CHDIR 1)
set(DAEMON_NO_CLOSE_STDIO 0)

set(SERVICE_SOURCES
  src/service/daemon.cc
  src/service/ipc_property_internal.cc
  src/service/ipc_property_service.cc
  )

add_executable(${IPC_PROP_SERVICE} ${SERVICE_SOURCES})

target_include_directories(${IPC_PROP_SERVICE} PUBLIC
  src/lib
  src/service
  include
  json
  )

target_link_libraries(${IPC_PROP_SERVICE}
        pthread)

target_compile_definitions(${IPC_PROP_SERVICE}
  PRIVATE DAEMON_NAME="${DAEMON_NAME}"
  PRIVATE DAEMON_MAJOR_VERSION="${DAEMON_MAJOR_VERSION}"
  PRIVATE DAEMON_MINOR_VERSION="${DAEMON_MINOR_VERSION}"
  PRIVATE DAEMON_PATCH_VERSION="${DAEMON_PATCH_VERSION}"
  PRIVATE DAEMON_PID_FILE_NAME="${DAEMON_PID_FILE_NAME}"
  PRIVATE DAEMON_LOG_FILE_NAME="${DAEMON_LOG_FILE_NAME}"
  PRIVATE DAEMON_NO_CHDIR=${DAEMON_NO_CHDIR}
  PRIVATE DAEMON_NO_CLOSE_STDIO=${DAEMON_NO_CLOSE_STDIO}
  )


#
# ipc property client
#
set(CLIENT_SOURCES
  src/client/ipc_property.c
  )

add_executable(${IPC_PROP_CLIENT} ${CLIENT_SOURCES})
target_include_directories(${IPC_PROP_CLIENT} PUBLIC
  include
  )

target_link_libraries(${IPC_PROP_CLIENT}
        ipc-property)

set_target_properties(${IPC_PROP_CLIENT}
  PROPERTIES OUTPUT_NAME ipc-property)
