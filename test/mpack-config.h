
#ifndef MPACK_CONFIG_H
#define MPACK_CONFIG_H 1

// This is the configuration for the MPack test harness.

#define MPACK_UNIT_TESTS 1

#if defined(DEBUG) || defined(_DEBUG)
#define MPACK_DEBUG 1
#endif

#ifdef MPACK_SCONS
    // Most options such as featureset and platform configuration
    // are specified by the SCons buildsystem. Any options that are
    // unset on the command line are considered disabled.
    #ifndef MPACK_READER
    #define MPACK_READER 0
    #endif
    #ifndef MPACK_EXPECT
    #define MPACK_EXPECT 0
    #endif
    #ifndef MPACK_NODE
    #define MPACK_NODE 0
    #endif
    #ifndef MPACK_WRITER
    #define MPACK_WRITER 0
    #endif
    #ifndef MPACK_STDLIB
    #define MPACK_STDLIB 0
    #endif
    #ifndef MPACK_STDIO
    #define MPACK_STDIO 0
    #endif
#else
    // For other platforms, we currently only test in the default
    // configuration, so we use the default for most settings.
    #define MPACK_MALLOC test_malloc
    #define MPACK_FREE test_free
#endif

// We replace the file i/o functions to simulate failures
#if MPACK_STDIO
#include <stdio.h>
#define fopen  test_fopen
#define fclose test_fclose
#define fread  test_fread
#define fwrite test_fwrite
#define fseek  test_fseek
#define ftell  test_ftell
#define ferror test_ferror
#endif

// Tracking matches the default config, except the test suite
// also supports MPACK_NO_TRACKING to disable it.
#if defined(MPACK_MALLOC) && !defined(MPACK_NO_TRACKING)
    #if defined(MPACK_DEBUG) && MPACK_DEBUG && defined(MPACK_READER) && MPACK_READER
        #define MPACK_READ_TRACKING 1
    #endif
    #if defined(MPACK_DEBUG) && MPACK_DEBUG && defined(MPACK_WRITER) && MPACK_WRITER
        #define MPACK_WRITE_TRACKING 1
    #endif
#endif

// We use a custom assert function which longjmps, allowing
// us to test assertions in debug mode.
#ifdef MPACK_DEBUG
#define MPACK_CUSTOM_ASSERT 1
#define MPACK_CUSTOM_BREAK 1
#endif

#include "test-system.h"

// we use small buffer sizes to test flushing, growing, and malloc failures
#define MPACK_TRACKING_INITIAL_CAPACITY 3
#define MPACK_STACK_SIZE 7
#define MPACK_BUFFER_SIZE 32
#define MPACK_NODE_PAGE_SIZE 113

#ifdef MPACK_MALLOC
#define MPACK_NODE_INITIAL_DEPTH 3
#else
#define MPACK_NODE_MAX_DEPTH_WITHOUT_MALLOC 32
#endif

#endif

