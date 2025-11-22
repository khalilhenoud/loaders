cmake_minimum_required(VERSION 3.22)

project(loaders_package)

if (HAS_STANDALONE_PREDECESSOR)
add_subdirectory(loaders)
else()
add_subdirectory(loaders_test)
endif()