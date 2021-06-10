cmake_minimum_required(VERSION 3.3)
cmake_policy(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(CheckIncludeFile)

project(lci C)

add_subdirectory(doc)

# default type is Release, change with cmake -DCMAKE_BUILD_TYPE=
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")	# common flags

# sources
file(GLOB SOURCES src/*.c src/**/*.c)

add_executable(lci ${SOURCES})
target_include_directories(lci PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/k08)

# search for readline
find_package(Readline)

if (READLINE_FOUND)
	add_compile_definitions(HAS_READLINE)
	target_link_libraries(lci readline)

	# not sure if this is needed
	# if (HISTORY_TRUNCATE_FILE_FOUND)
	# 	add_compile_definitions(HAVE_HISTORY_TRUNCATE_FILE)
	# endif(
endif()

# search for sys/ioctl.h
check_include_file(sys/ioctl.h IOCTL_FOUND)

if (IOCTL_FOUND)
	add_compile_definitions(HAS_IOCTL)
endif()

# copy some files to the build dir for easier debugging (configure_file has the advantage of
# creating a dependency, modifying the source file will update the target).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/.lcirc     ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/queens.lci ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# installation
install(TARGETS lci RUNTIME)
install(FILES src/.lcirc src/queens.lci DESTINATION share/lci)

# simple test
add_custom_target(test
	DEPENDS lci
	COMMAND printf "Consult 'queens.lci'\\nQueens 5" | ./lci
	VERBATIM
)

# Print all variables
if(VERBOSE)
	get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
		message(STATUS "${_variableName}=${${_variableName}}")
	endforeach()
endif()
