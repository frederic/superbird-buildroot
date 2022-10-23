cmake_minimum_required(VERSION 3.8)

#############################################
# libcutils
#############################################
add_library(cutils SHARED
        hashmap.cpp
        str_parms.cpp
        threads.cpp
        properties.cpp
)

target_include_directories(cutils PRIVATE
	./include)

add_library(cutils_static STATIC
	strlcpy.c)

target_include_directories(cutils_static PRIVATE
	./include)

target_link_libraries(cutils
	-Wl,--whole-archive cutils_static -Wl,--no-whole-archive)

install(TARGETS cutils
        LIBRARY DESTINATION "lib")
