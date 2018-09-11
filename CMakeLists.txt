cmake_minimum_required(VERSION 3.10)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt and CMakeFiles/*." )
endif()

project(ncmdump CXX)

set(CMAKE_CXX_STANDARD 17)

include("cmake/ucm.cmake")
ucm_set_runtime(STATIC)

add_executable(ncmdump
    src/main.cpp
    src/decrypt.h
    src/decrypt.cpp
    src/tagger.h
    src/tagger.cpp)

if(UNIX)
    # detect standard library
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles(
        "#include <iostream>
        int a =
        #ifdef __GLIBCXX__
            1;
        #endif
        int main(int argc, char* argv[])
        {
            return 0;
        }"
        USE_LIBSTDCXX)

    if(${USE_LIBSTDCXX})
        target_link_libraries(ncmdump PRIVATE stdc++fs)
    else() # use libc++
        target_link_libraries(ncmdump PRIVATE c++experimental)
    endif()
endif()

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(ncmdump PRIVATE jsoncpp_lib_static)

find_package(OpenSSL REQUIRED)
target_link_libraries(ncmdump PRIVATE OpenSSL::Crypto)

find_library(TagLib tag)
if(${TagLib-NOTFOUND})
    message(FATAL_ERROR "cannot find TagLib")
endif()
target_compile_definitions(ncmdump PRIVATE TAGLIB_STATIC=)
target_link_libraries(ncmdump PRIVATE "${TagLib}")

find_package(ZLIB REQUIRED)
target_link_libraries(ncmdump PRIVATE ZLIB::ZLIB)
