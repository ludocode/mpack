cmake_minimum_required(VERSION 2.6)
project(mpack)

set(MPACK_MAJOR 0)
set(MPACK_MINOR 7)

# mpack features
set(MPACK_READER  1)
set(MPACK_EXPECT  1)
set(MPACK_NODE    1)
set(MPACK_WRITER  1)
# mpack dependencies
set(MPACK_STDLIB  1)
set(MPACK_STDIO   1)
set(MPACK_SETJMP  1)
# mpack miscellaneous
set(MPACK_STACK_SIZE               4096)
set(MPACK_BUFFER_SIZE              4096)
set(MPACK_NODE_ARRAY_STARTING_SIZE 32)
set(MPACK_NODE_MAX_DEPTH           2048)

# compiler options
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99")

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_BINARY_DIR}/src"
)

# miscellaneous
set(MPACK_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/mpack")
set(MPACK_DOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")

enable_testing()

# load cmake packages
find_package(Doxygen)
if(DOXYGEN_FOUND)
	file(MAKE_DIRECTORY "${MPACK_DOC_DIR}")
	configure_file (
		"Doxyfile.in"
		"${MPACK_DOC_DIR}/Doxyfile"
	)
	add_custom_target(doc ALL
		${DOXYGEN_EXECUTABLE} ${MPACK_DOC_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_DOC_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)

# valgrind
find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )

# include source and test folders
add_subdirectory("src")
add_subdirectory("test")
