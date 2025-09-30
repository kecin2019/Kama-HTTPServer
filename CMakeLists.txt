cmake_minimum_required(VERSION 3.10)
project(MultiApps)

option(BUILD_AI_APPS "Build AI Apps" OFF)
option(BUILD_WEB_APPS "Build Web Apps" ON)

if(BUILD_AI_APPS)
    message(STATUS "Building AI Apps ...")
    include(${CMAKE_SOURCE_DIR}/AIAppsCMakeLists.txt)
endif()

if(BUILD_WEB_APPS)
    message(STATUS "Building Web Apps ...")
    include(${CMAKE_SOURCE_DIR}/WebAppsCMakeLists.txt)
endif()