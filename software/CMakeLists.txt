cmake_minimum_required(VERSION 3.24)

project(HomeAutomationInterface LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HAI_FETCH_DEPS "Fetch missing third-party dependencies with CMake FetchContent" ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(application)
