cmake_minimum_required(VERSION 3.3)

project(lci C)

add_subdirectory(dparser EXCLUDE_FROM_ALL)
add_subdirectory(replxx EXCLUDE_FROM_ALL)
add_subdirectory(doc)
add_subdirectory(html)

# default type is Release, change with cmake -DCMAKE_BUILD_TYPE=
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")	# common flags

# generate parser
# NOTE: under emscripten, make_dparser.js is generated, so we need to have dparser/make_dparser from a previous run.
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/grammar.g.d_parser.c
	DEPENDS make_dparser src/grammar.g
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM
	COMMAND dparser/make_dparser ${CMAKE_CURRENT_SOURCE_DIR}/src/grammar.g -o ${CMAKE_CURRENT_BINARY_DIR}/grammar.g.d_parser.c
)

# lci executable
file(GLOB SOURCES src/*.c src/**/*.c)
add_executable(lci ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/grammar.g.d_parser.c)

target_include_directories(lci PUBLIC src src/k08 dparser)
target_link_libraries(lci dparse)

# set emcscriptenoptions and copy all html/* files to the build dir
if(EMSCRIPTEN)
	# Note: SHELL: is needed otherwise cmake merges "-s foo -s bar" into "-s foo bar" !!
	target_link_options(lci PUBLIC "SHELL:-s ALLOW_MEMORY_GROWTH" "SHELL:-s ASYNCIFY" "SHELL:-s ASYNCIFY_IMPORTS=[js_read_line,js_read_char,js_read_ctrl_c]" --preload-file=.lcirc --preload-file=queens.lci)
else()
	# no emscripten, we need replxx. stdc++ is needed by replxx (static link on windows to avoid dependencies).
	target_link_libraries(lci replxx $<IF:$<BOOL:${WIN32}>,-static,> stdc++)
endif()

# if present, lci tries to read global .lcirc from DATADIR
if(CMAKE_INSTALL_PREFIX)
	target_compile_definitions(lci PUBLIC DATADIR="${CMAKE_INSTALL_PREFIX}/share")
endif()

# copy some files to the build dir for easier debugging (configure_file has the advantage of
# creating a dependency, modifying the source file will update the target).
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/.lcirc     ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/queens.lci ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# installation
install(TARGETS lci RUNTIME)
install(FILES src/.lcirc src/queens.lci DESTINATION share/lci)

# prepare binary release
add_custom_target(release_bin
	DEPENDS lci
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} VERBATIM
	COMMAND ${CMAKE_COMMAND} -E make_directory release_bin
	COMMAND ${CMAKE_COMMAND} -E copy .lcirc queens.lci $<TARGET_FILE:lci> release_bin/
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/README.md ${CMAKE_CURRENT_SOURCE_DIR}/COPYING release_bin/
)

# simple tests
enable_testing()
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")

add_test(build make lci)		# make sure lci is build, strangly I couldn't find a way to add a dependency
add_test(queens
	sh -c "printf \"Consult 'queens.lci'; Queens 5\\n\" | ./lci"
)
set_tests_properties(queens PROPERTIES
	PASS_REGULAR_EXPRESS[I,1]ON "\\[\\[[I,1], 3, 5, 2, 4\\], \\[[I,1], 4, 2, 5, 3\\], \\[2, 4, [I,1], 3, 5\\], \\[2, 5, 3, [I,1], 4\\], \\[3, [I,1], 4, 2, 5\\], \\[3, 5, 2, 4, [I,1]\\], \\[4, [I,1], 3, 5, 2\\], \\[4, 2, 5, 3, [I,1]\\], \\[5, 2, 4, [I,1], 3\\], \\[5, 3, [I,1], 4, 2\\]\\]"
)

add_test(fixed_point
	sh -c "printf \"FixedPoint; Append [3,4,[]] [5]\\n\" | ./lci"
)
set_tests_properties(fixed_point PROPERTIES
	PASS_REGULAR_EXPRESSION "cycles removed.*\\[3, 4, \\[\\], 5\\]"
)

# valgrind test
find_program(VALGRIND valgrind)
if(VALGRIND)
	add_test(valgrind
		sh -c "printf \"FixedPoint; Append [3,4,[]] [5]\\n\" | valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all ./lci"
	)
endif()

# Print all variables
if(VERBOSE)
	get_cmake_property(_variableNames VARIABLES)
	foreach (_variableName ${_variableNames})
		message(STATUS "${_variableName}=${${_variableName}}")
	endforeach()
endif()
