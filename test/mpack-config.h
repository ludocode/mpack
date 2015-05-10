
#ifndef MPACK_CONFIG_H
#define MPACK_CONFIG_H 1

// This is the configuration for the MPack test harness.
// Note that most options such as featureset and platform configuration
// are instead specified by the buildsystem.

#if defined(DEBUG) || defined(_DEBUG)
#define MPACK_DEBUG 1
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

// we use small buffer sizes to test flushing and malloc failures
#define MPACK_STACK_SIZE 7
#define MPACK_BUFFER_SIZE 7
#define MPACK_NODE_ARRAY_STARTING_SIZE 32
#define MPACK_NODE_MAX_DEPTH 2048

// don't include debug print functions in code coverage
#define MPACK_NO_PRINT 1

#endif

