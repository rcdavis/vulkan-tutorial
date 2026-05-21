cmake_minimum_required(VERSION 3.14)

project(VulkanTutorial VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/dependencies.cmake)

add_subdirectory(App)
