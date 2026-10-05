cmake_minimum_required(VERSION 3.25)

project(emberdeep LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

add_subdirectory(core)
option(EMBERDEEP_BUILD_CLIENT "Build graphical and terminal clients" ON)
if(EMBERDEEP_BUILD_CLIENT)
  add_subdirectory(client)
endif()
add_subdirectory(server)
add_subdirectory(tests)
