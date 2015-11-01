set(MPACK_TEST
	"${MPACK_SRC}"
	test.c
	test-buffer.c
	test-common.c
	test-expect.c
	test-file.c
	test-node.c
	test-reader.c
	test-system.c
	test-write.c
)

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

file(COPY test-file.debug test-file.mp test-file.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_executable(mpack_test ${MPACK_TEST})
add_test(NAME test COMMAND ${CMAKE_BINARY_DIR}/test/mpack_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
