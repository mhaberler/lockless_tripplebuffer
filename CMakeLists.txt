#-------------------------------------------------------------------------------
#
# Copyright 2014, Adrian Gierakowski and p4checo (https://github.com/p4checo)
#
# A lockless single writer, single reader triple buffer
# Implemented using C++11 atomic operations

# This class can be used to transport data between two threads
# it doesn't guarantee that all data will be transported
# however it can guarantee access to a complete (predefined) chunk of data

# for example: when used to transport audio samples from audio thread to graphics
# thread, we can ensure that the graphics thread is always accessing a block
# of CONSECUTIVE samples and that this block is the block which was most recently
# filled by the audio thread.

# this is a slightly modified version of implementation published here:
# https://github.com/p4checo/triplebuffer-sync
# which in turn was based on following blog post:
# http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html

# my contributions:
# added by-reference access to the content of the buffer
# added a multithreaded unit test (using gmock)
# packaged this code as a header only CMAKE project
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)

project(lockless_tripplebuffer)

set(MYLIB_NAME "lockless_tripplebuffer")
set(MYLIB_VERSION "0.1.0")
set(MYLIB_BRIEF "${MYLIB_NAME} is an implementation of a lockless tripple buffer")


option(MYLIB_TESTS_ENABLED "Enable tests build" ON)


if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.6.3")
    message(FATAL_ERROR "g++ 4.6.3 or later required")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "4.7")
    set(CMAKE_CXX_FLAGS "-std=c++11")
else()
    # Set GTEST_LANG_CXX11=0 to disable C++11 features when compiling googlemock.
    # Without this compilation fails with g++ 4.6.3 on gmock-matchers.h.
    set(CMAKE_CXX_FLAGS "-std=c++0x -DGTEST_LANG_CXX11=0")
endif()

set(COMMON_FLAGS "-Wall -Wextra -Wconversion -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")

if(APPLE)
    set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()


add_library(${MYLIB_NAME} INTERFACE)
# set_property(TARGET ${MYLIB_NAME}
#   APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
#   ${PROJECT_SOURCE_DIR}/include
# )

target_include_directories(${MYLIB_NAME}
  INTERFACE ${PROJECT_SOURCE_DIR}/include)

if(MYLIB_TESTS_ENABLED)
  add_subdirectory(test)
endif()
