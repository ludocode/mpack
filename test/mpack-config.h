
#ifndef MPACK_CONFIG_H
#define MPACK_CONFIG_H 1

// This is the configuration for the MPack test harness.

#if defined(DEBUG) || defined(_DEBUG)
#define MPACK_DEBUG 1
#define MPACK_TRACKING 1
#endif

#define MPACK_READER 1
#define MPACK_WRITER 1
#define MPACK_EXPECT 1
#define MPACK_NODE 1

#define MPACK_STDLIB 1
#define MPACK_STDIO 1
#define MPACK_SETJMP 1

#include "test-malloc.h"
#define MPACK_MALLOC test_malloc
#define MPACK_FREE test_free

// the test harness uses a custom assert function since we
// test whether assertions are hit
#define MPACK_CUSTOM_ASSERT 1

#define MPACK_STACK_SIZE 4096
#define MPACK_BUFFER_SIZE 4096
#define MPACK_NODE_ARRAY_STARTING_SIZE 32
#define MPACK_NODE_MAX_DEPTH 2048

// don't include debug print functions in code coverage
#define MPACK_NO_PRINT 1

#endif

