cmake_minimum_required(VERSION 3.22)

# all libraries built by this project are copied into the output directory.
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# set the project name
project(loaders_test VERSION 1.0)

# DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	# Update submodules as needed
	option(GIT_SUBMODULE "Check submodules during build" ON)
	if(GIT_SUBMODULES)
		message(STATUS "Submodule update")
		execute_process(COMMAND $(GIT_EXECUTABLE) submodule upate --init --recursive 
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}")
		endif()
	endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/library/CMakeLists.txt")
	message(FATAL_ERROR "The library submdule was not downloaded!")
endif()

add_subdirectory(external/library library)
add_subdirectory(../loaders loaders)

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} 
				./source/main.cpp
				)
			
target_link_libraries(${PROJECT_NAME}
						PRIVATE loaders
						)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)