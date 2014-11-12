#-------------------------------------------------------------------------------
#
# Copyright 2014, Adrian Gierakowski and p4checo (https://github.com/p4checo)
#
# Test suite for a lockless single writer, single reader tripple buffer
# Implemented using C++11 atomic operations
#
# This class can be used to transport data between two threads
#
# this is a slightly modified version of implementation published here:
# https://github.com/p4checo/triplebuffer-sync
# which in turn was based on following blog post:
# http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html
#
#
# my contribution was mainly to add a multithreaded unit test (using gmock)
# and package this code as a header only CMAKE project
#
# Author: Adrian Gierakowski
#
#-------------------------------------------------------------------------------


enable_testing()

# gtest definition
include(ExternalProject)

ExternalProject_Add(
    gtest
    SVN_REPOSITORY http://googletest.googlecode.com/svn/tags/release-1.7.0
    TIMEOUT 10
    # Disable install step
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    PREFIX "gtest"
)

ExternalProject_Get_Property(gtest source_dir)
ExternalProject_Get_Property(gtest binary_dir)

# tests definition
set(TARGET lockless_tripplebuffer)
set(GTEST_TARGET ${TARGET}_GTest)
file(GLOB_RECURSE TESTS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cc)

message("TESTS_FILES: ${TESTS_FILES}")

message("source_dir: ${source_dir}")
message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("binary_dir: ${binary_dir}")

add_executable(${GTEST_TARGET} ${TESTS_FILES})
include_directories(
  ${source_dir}/include
)

add_dependencies(${GTEST_TARGET} gtest)
target_link_libraries(${GTEST_TARGET}
  ${TARGET}
# TODO OP should work but with something around ENABLE_EXPORTS
#  gtest gtest_main
  # ${binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
  # ${binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
)

# TODO: specific paths for Debug and Release might have to
# be also put in place for MSVC
if(CMAKE_GENERATOR MATCHES "Xcode")
  message("CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
  target_link_libraries( ${GTEST_TARGET} debug
    ${binary_dir}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
  target_link_libraries( ${GTEST_TARGET} debug
    ${binary_dir}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
  target_link_libraries( ${GTEST_TARGET} optimized
    ${binary_dir}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
  target_link_libraries( ${GTEST_TARGET} optimized
    ${binary_dir}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
  target_link_libraries( ${GTEST_TARGET}
    ${binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
  target_link_libraries( ${GTEST_TARGET}
    ${binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()


if(CMAKE_COMPILER_IS_GNUC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(${GTEST_TARGET} pthread)
endif()

