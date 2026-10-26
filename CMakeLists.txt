cmake_minimum_required(VERSION 3.29)
project(taft VERSION 0.0.1 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug")
endif()
message ("${PROJECT_NAME}: Build type set to ${CMAKE_BUILD_TYPE}")

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(TAFT_IS_TOP_LEVEL ON)
else()
    set(TAFT_IS_TOP_LEVEL OFF)
endif()

set(TAFT_BUILD_EXECUTABLE_DEFAULT ON)
if(DEFINED BUILD_LIBARIES_ONLY AND BUILD_LIBARIES_ONLY)
    set(TAFT_BUILD_EXECUTABLE_DEFAULT OFF)
endif()
option(TAFT_BUILD_EXECUTABLE "Build the TAFT executable" ${TAFT_BUILD_EXECUTABLE_DEFAULT})
if(DEFINED BUILD_LIBARIES_ONLY AND BUILD_LIBARIES_ONLY)
    message(STATUS "BUILD_LIBARIES_ONLY is deprecated; use TAFT_BUILD_EXECUTABLE instead")
endif()
if(NOT TAFT_BUILD_EXECUTABLE)
    message(STATUS "${PROJECT_NAME}: only building libraries, skipping executable")
endif()

set(TAFT_BUILD_TESTING_DEFAULT OFF)
if(TAFT_IS_TOP_LEVEL)
    set(TAFT_BUILD_TESTING_DEFAULT ON)
    if(DEFINED BUILD_TESTING)
        set(TAFT_BUILD_TESTING_DEFAULT ${BUILD_TESTING})
    endif()
endif()
option(TAFT_BUILD_TESTING "Build ${PROJECT_NAME} tests" ${TAFT_BUILD_TESTING_DEFAULT})
if(NOT TAFT_BUILD_TESTING)
    message(STATUS "${PROJECT_NAME}: skipping test targets")
endif()

# Get CPM
include(${PROJECT_SOURCE_DIR}/.cmake/get_cpm.cmake)

# Include a suite of helper functions for output and building the aaesim package
include(${PROJECT_SOURCE_DIR}/.cmake/functions.cmake)

CPMAddPackage(
  NAME Ccache.cmake
  GITHUB_REPOSITORY TheLartians/Ccache.cmake
  VERSION 1.2.5
)

set(TAFT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
message("-- Generating build for ${PROJECT_NAME} version ${TAFT_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wno-unused-function -Wno-sign-compare -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wno-unused-function -Wno-sign-compare -O3 -g3")

if(NOT TARGET mitre::fsloader)
    CPMAddPackage(
        NAME fsloader
        GIT_REPOSITORY https://github.com/mitre/fsloader.git
        GIT_TAG 1.1.0
        OPTIONS
         "FSLOADER_BUILD_EXAMPLE OFF"
         "BUILD_SHARED_LIBS FALSE"
    )
endif()

if(NOT TARGET mitre::oss::simcore)
    CPMAddPackage(
        NAME aircraft_simulation_core
        GIT_REPOSITORY https://github.com/mitre/aircraft_simulation_core.git
        GIT_TAG 1.0.0
        OPTIONS
         "SIMCORE_BUILD_TESTING OFF"
    )
endif()

set (FRAMEWORK_DIR       ${CMAKE_CURRENT_SOURCE_DIR}/AircraftDynamicsTestFramework)
set (aaesim_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include )
set (UNITTEST_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/unittest)

include(${FRAMEWORK_DIR}/framework.cmake OPTIONAL)
if(TAFT_BUILD_TESTING)
    include(${UNITTEST_DIR}/unittest.cmake OPTIONAL)
endif()
