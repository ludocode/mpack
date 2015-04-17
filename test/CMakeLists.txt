set(MPACK_TEST
	main.c
	test-buffer.c
	test-file.c
	test-malloc.c
	test-node.c
	test-read.c
	test-tag.c
	test-write.c
)

add_executable(mpack_test ${MPACK_TEST})
target_link_libraries(mpack_test mpack-static)
