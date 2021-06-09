cmake_minimum_required(VERSION 3.3)
cmake_policy(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

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
file(GLOB SOURCES src/*.c src/kazlib/*.c)

add_executable(lci ${SOURCES})

# search for readline
find_package(Readline)

if (READLINE_FOUND)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
	message(STATUS "Readline found")

  add_compile_definitions(USE_READLINE)
  target_link_libraries(lci readline)
endif()

# copy some files to the build dir for easier debugging
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/.lcirc     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/queens.lci DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# installation
install(TARGETS lci RUNTIME)
install(FILES src/.lcirc src/queens.lci DESTINATION share/lci)

# Print all variables
if(VERBOSE)
	get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
		message(STATUS "${_variableName}=${${_variableName}}")
	endforeach()
endif()
