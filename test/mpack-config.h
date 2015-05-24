
#ifndef MPACK_CONFIG_H
#define MPACK_CONFIG_H 1

// This is the configuration for the MPack test harness.

#if defined(DEBUG) || defined(_DEBUG)
#define MPACK_DEBUG 1
#endif

// Most options such as featureset and platform configuration
// are specified by the SCons buildsystem. For other platforms,
// we define the usual configuration here.
#ifndef MPACK_SCONS
    #define MPACK_READER 1
    #define MPACK_WRITER 1
    #define MPACK_EXPECT 1
    #define MPACK_NODE 1

    #define MPACK_STDLIB 1
    #define MPACK_STDIO 1
    #define MPACK_SETJMP 1
    #define MPACK_MALLOC test_malloc
    #define MPACK_FREE test_free
#endif

#if defined(MPACK_MALLOC) && !defined(MPACK_NO_TRACKING)
#define MPACK_TRACKING 1
#endif

#ifdef MPACK_MALLOC
#include "test-malloc.h"
#endif

// the test harness uses a custom assert function since we
// test whether assertions are hit
#define MPACK_CUSTOM_ASSERT 1

// we use small buffer sizes to test flushing, growing, and malloc failures
#define MPACK_STACK_SIZE 7
#define MPACK_BUFFER_SIZE 7
#define MPACK_NODE_ARRAY_STARTING_SIZE 32
#define MPACK_NODE_MAX_DEPTH 2048
#define MPACK_TRACKING_INITIAL_CAPACITY 1
#define MPACK_NODE_PAGE_INITIAL_CAPACITY 1
#define MPACK_NODE_PAGE_SIZE 2

// don't include debug print functions in code coverage
#define MPACK_NO_PRINT 1

#endif

