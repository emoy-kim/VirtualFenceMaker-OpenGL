﻿cmake_minimum_required(VERSION 3.8)

project("VirtualFenceMakerGL")

include(cmake/check-compiler.cmake)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp VirtualFenceMakerGL.cpp)

configure_file(ProjectPath.h.in ${PROJECT_BINARY_DIR}/ProjectPath.h @ONLY)

if(MSVC)
   include(cmake/add-libraries-windows.cmake)
else()
   include(cmake/add-libraries-linux.cmake)
endif()

add_executable(VirtualFenceMakerGL ${SOURCE_FILES})

if(MSVC)
   include(cmake/target-link-libraries-windows.cmake)
else()
   include(cmake/target-link-libraries-linux.cmake)
endif()

target_include_directories(VirtualFenceMakerGL PUBLIC ${CMAKE_BINARY_DIR})