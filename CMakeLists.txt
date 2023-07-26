cmake_minimum_required(VERSION 3.22)

project(loaders_package)

if(ENGINE_BUILD)
add_subdirectory(loaders)
else()
add_subdirectory(loaders_test)
endif()