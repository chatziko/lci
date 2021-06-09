PROJECT(lci C)
cmake_minimum_required(VERSION 2.8)

# ignore if latex is not installed
find_program(LATEX "latx")
IF(NOT LATEX)
	return()
ENDIF()

INCLUDE(../cmake/UseLATEX.cmake)

ADD_LATEX_DOCUMENT(lcidoc.tex)
ADD_LATEX_DOCUMENT(lcidoc_gr.tex)

# don't build the doc when running just "make"
set_target_properties(lcidoc PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(lcidoc_gr PROPERTIES EXCLUDE_FROM_ALL TRUE)

# target to make the html version
add_custom_target(html
	SOURCES lcidoc.tex
	COMMAND hevea ${CMAKE_CURRENT_SOURCE_DIR}/lcidoc.tex
	COMMAND hevea ${CMAKE_CURRENT_SOURCE_DIR}/lcidoc.tex
)

# 'doc' target, depends on all docs
add_custom_target(doc)
add_dependencies(doc lcidoc lcidoc_gr html)