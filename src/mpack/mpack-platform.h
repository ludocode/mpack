/*
 * Copyright (c) 2015-2021 Nicholas Fraser and the MPack authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file
 *
 * Abstracts all platform-specific code from MPack and handles configuration
 * options.
 *
 * This verifies the configuration and sets defaults based on the platform,
 * contains implementations of standard C functions when libc is not available,
 * and provides wrappers to all library functions.
 *
 * Documentation for configuration options is available here:
 *
 *     https://ludocode.github.io/mpack/group__config.html
 */

#ifndef MPACK_PLATFORM_H
#define MPACK_PLATFORM_H 1



/**
 * @defgroup config Configuration Options
 *
 * Defines the MPack configuration options.
 *
 * Custom configuration of MPack is not usually necessary. In almost all
 * cases you can ignore this and use the defaults.
 *
 * If you do want to configure MPack, you can define the below options as part
 * of your build system or project settings. This will override the below
 * defaults.
 *
 * If you'd like to use a file for configuration instead, define
 * @ref MPACK_HAS_CONFIG to 1 in your build system or project settings.
 * This will cause MPack to include a file you create called @c mpack-config.h
 * in which you can define your configuration. This is useful if you need to
 * include specific headers (such as a custom allocator) in order to configure
 * MPack to use it.
 *
 * @warning The value of all configuration options must be the same in
 * all translation units of your project, as well as in the mpack source
 * itself. These configuration options affect the layout of structs, among
 * other things, which cannot be different in source files that are linked
 * together.
 *
 * @note MPack does not contain defaults for building inside the Linux kernel.
 * There is a <a href="https://github.com/ludocode/mpack-linux-kernel">
 * configuration file for the Linux kernel</a> that can be used instead.
 *
 * @{
 */



/*
 * Pre-include checks
 *
 * These need to come before the user's mpack-config.h because they might be
 * including headers in it.
 */

/** @cond */
#if defined(_MSC_VER) && _MSC_VER < 1800 && !defined(__cplusplus)
    #error "In Visual Studio 2012 and earlier, MPack must be compiled as C++. Enable the /Tp compiler flag."
#endif

#if defined(_WIN32) && MPACK_INTERNAL
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

#ifndef __STDC_LIMIT_MACROS
    #define __STDC_LIMIT_MACROS 1
#endif
#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS 1
#endif
#ifndef __STDC_CONSTANT_MACROS
    #define __STDC_CONSTANT_MACROS 1
#endif
/** @endcond */



/**
 * @name File Configuration
 * @{
 */

/**
 * @def MPACK_HAS_CONFIG
 *
 * Causes MPack to include a file you create called @c mpack-config.h .
 *
 * The file is included before MPack sets any defaults for undefined
 * configuration options. You can use it to configure MPack.
 *
 * This is off by default.
 */
#if defined(MPACK_HAS_CONFIG)
    #if MPACK_HAS_CONFIG
        #include "mpack-config.h"
    #endif
#else
    #define MPACK_HAS_CONFIG 0
#endif

/**
 * @}
 */

// this needs to come first since some stuff depends on it
/** @cond */
#ifndef MPACK_NO_BUILTINS
    #define MPACK_NO_BUILTINS 0
#endif
/** @endcond */



/**
 * @name Features
 * @{
 */

/**
 * @def MPACK_READER
 *
 * Enables compilation of the base Tag Reader.
 */
#ifndef MPACK_READER
#define MPACK_READER 1
#endif

/**
 * @def MPACK_EXPECT
 *
 * Enables compilation of the static Expect API.
 */
#ifndef MPACK_EXPECT
#define MPACK_EXPECT 1
#endif

/**
 * @def MPACK_NODE
 *
 * Enables compilation of the dynamic Node API.
 */
#ifndef MPACK_NODE
#define MPACK_NODE 1
#endif

/**
 * @def MPACK_WRITER
 *
 * Enables compilation of the Writer.
 */
#ifndef MPACK_WRITER
#define MPACK_WRITER 1
#endif

/**
 * @def MPACK_BUILDER
 *
 * Enables compilation of the Builder.
 *
 * The Builder API provides additional functions to the Writer for
 * automatically determining the element count of compound elements so you do
 * not have to specify them up-front.
 *
 * This requires a @c malloc(). It is enabled by default if MPACK_WRITER is
 * enabled and MPACK_MALLOC is defined.
 *
 * @see mpack_build_map()
 * @see mpack_build_array()
 * @see mpack_complete_map()
 * @see mpack_complete_array()
 */
// This is defined furthur below after we've resolved whether we have malloc().

/**
 * @def MPACK_COMPATIBILITY
 *
 * Enables compatibility features for reading and writing older
 * versions of MessagePack.
 *
 * This is disabled by default. When disabled, the behaviour is equivalent to
 * using the default version, @ref mpack_version_current.
 *
 * Enable this if you need to interoperate with applications or data that do
 * not support the new (v5) MessagePack spec. See the section on v4
 * compatibility in @ref docs/protocol.md for more information.
 */
#ifndef MPACK_COMPATIBILITY
#define MPACK_COMPATIBILITY 0
#endif

/**
 * @def MPACK_EXTENSIONS
 *
 * Enables the use of extension types.
 *
 * This is disabled by default. Define it to 1 to enable it. If disabled,
 * functions to read and write extensions will not exist, and any occurrence of
 * extension types in parsed messages will flag @ref mpack_error_invalid.
 *
 * MPack discourages the use of extension types. See the section on extension
 * types in @ref docs/protocol.md for more information.
 */
#ifndef MPACK_EXTENSIONS
#define MPACK_EXTENSIONS 0
#endif

/**
 * @}
 */



// workarounds for Doxygen
#if defined(MPACK_DOXYGEN)
#if MPACK_DOXYGEN
// We give these their default values of 0 here even though they are defined to
// 1 in the doxyfile. Doxygen will show this as the value in the docs, even
// though it ignores it when parsing the rest of the source. This is what we
// want, since we want the documentation to show these defaults but still
// generate documentation for the functions they add when they're on.
#define MPACK_COMPATIBILITY 0
#define MPACK_EXTENSIONS 0
#endif
#endif



/**
 * @name Dependencies
 * @{
 */

/**
 * @def MPACK_CONFORMING
 *
 * Enables the inclusion of basic C headers to define standard types and
 * macros.
 *
 * This causes MPack to include headers required for conforming implementations
 * of C99 even in freestanding, in particular <stddef.h>, <stdint.h>,
 * <stdbool.h> and <limits.h>. It also includes <inttypes.h>; this is
 * technically not required for freestanding but MPack needs it to detect
 * integer limits.
 *
 * You can disable this if these headers are unavailable or if they do not
 * define the standard types and macros (for example inside the Linux kernel.)
 * If this is disabled, MPack will include no headers and will assume a 32-bit
 * int. You will probably also want to define @ref MPACK_HAS_CONFIG to 1 and
 * include your own headers in the config file. You must provide definitions
 * for standard types such as @c size_t, @c bool, @c int32_t and so on.
 *
 * @see <a href="https://en.cppreference.com/w/c/language/conformance">
 * cppreference.com documentation on Conformance</a>
 */
#ifndef MPACK_CONFORMING
    #define MPACK_CONFORMING 1
#endif

/**
 * @def MPACK_STDLIB
 *
 * Enables the use of the C stdlib.
 *
 * This allows the library to use basic functions like @c memcmp() and @c
 * strlen(), as well as @c malloc() for debugging and in allocation helpers.
 *
 * If this is disabled, allocation helper functions will not be defined, and
 * MPack will attempt to detect compiler intrinsics for functions like @c
 * memcmp() (assuming @ref MPACK_NO_BUILTINS is not set.) It will fallback to
 * its own (slow) implementations if it cannot use builtins. You may want to
 * define @ref MPACK_MEMCMP and friends if you disable this.
 *
 * @see MPACK_MEMCMP
 * @see MPACK_MEMCPY
 * @see MPACK_MEMMOVE
 * @see MPACK_MEMSET
 * @see MPACK_STRLEN
 * @see MPACK_MALLOC
 * @see MPACK_REALLOC
 * @see MPACK_FREE
 */
#ifndef MPACK_STDLIB
    #if !MPACK_CONFORMING
        // If we don't even have a proper <limits.h> we assume we won't have
        // malloc() either.
        #define MPACK_STDLIB 0
    #else
        #define MPACK_STDLIB 1
    #endif
#endif

/**
 * @def MPACK_STDIO
 *
 * Enables the use of C stdio. This adds helpers for easily
 * reading/writing C files and makes debugging easier.
 */
#ifndef MPACK_STDIO
    #if !MPACK_STDLIB || defined(__AVR__)
        #define MPACK_STDIO 0
    #else
        #define MPACK_STDIO 1
    #endif
#endif

/**
 * Whether the 'float' type and floating point operations are supported.
 *
 * If @ref MPACK_FLOAT is disabled, floats are read and written as @c uint32_t
 * instead. This way messages with floats do not result in errors and you can
 * still perform manual float parsing yourself.
 */
#ifndef MPACK_FLOAT
    #define MPACK_FLOAT 1
#endif

/**
 * Whether the 'double' type is supported. This requires support for 'float'.
 *
 * If @ref MPACK_DOUBLE is disabled, doubles are read and written as @c
 * uint32_t instead. This way messages with doubles do not result in errors and
 * you can still perform manual doubles parsing yourself.
 *
 * If @ref MPACK_FLOAT is enabled but @ref MPACK_DOUBLE is not, doubles can be
 * read as floats using the shortening conversion functions, e.g. @ref
 * mpack_expect_float() or @ref mpack_node_float().
 */
#ifndef MPACK_DOUBLE
    #if !MPACK_FLOAT || defined(__AVR__)
        // AVR supports only float, not double.
        #define MPACK_DOUBLE 0
    #else
        #define MPACK_DOUBLE 1
    #endif
#endif

/**
 * @}
 */



/**
 * @name Allocation Functions
 * @{
 */

/**
 * @def MPACK_MALLOC
 *
 * Defines the memory allocation function used by MPack. This is used by
 * helpers for automatically allocating data the correct size, and for
 * debugging functions. If this macro is undefined, the allocation helpers
 * will not be compiled.
 *
 * Set this to use a custom @c malloc() function. Your function must have the
 * signature:
 *
 * @code
 * void* malloc(size_t size);
 * @endcode
 *
 * The default is @c malloc() if @ref MPACK_STDLIB is enabled.
 */
/**
 * @def MPACK_FREE
 *
 * Defines the memory free function used by MPack. This is used by helpers
 * for automatically allocating data the correct size. If this macro is
 * undefined, the allocation helpers will not be compiled.
 *
 * Set this to use a custom @c free() function. Your function must have the
 * signature:
 *
 * @code
 * void free(void* p);
 * @endcode
 *
 * The default is @c free() if @ref MPACK_MALLOC has not been customized and
 * @ref MPACK_STDLIB is enabled.
 */
/**
 * @def MPACK_REALLOC
 *
 * Defines the realloc function used by MPack. It is used by growable
 * buffers to resize more efficiently.
 *
 * The default is @c realloc() if @ref MPACK_MALLOC has not been customized and
 * @ref MPACK_STDLIB is enabled.
 *
 * Set this to use a custom @c realloc() function. Your function must have the
 * signature:
 *
 * @code
 * void* realloc(void* p, size_t new_size);
 * @endcode
 *
 * This is optional, even when @ref MPACK_MALLOC is used. If @ref MPACK_MALLOC is
 * set and @ref MPACK_REALLOC is not, @ref MPACK_MALLOC is used with a simple copy
 * to grow buffers.
 */

#if defined(MPACK_MALLOC) && !defined(MPACK_FREE)
    #error "MPACK_MALLOC requires MPACK_FREE."
#endif
#if !defined(MPACK_MALLOC) && defined(MPACK_FREE)
    #error "MPACK_FREE requires MPACK_MALLOC."
#endif

// These were never configurable in lowercase but we check anyway.
#ifdef mpack_malloc
    #error "Define MPACK_MALLOC, not mpack_malloc."
#endif
#ifdef mpack_realloc
    #error "Define MPACK_REALLOC, not mpack_realloc."
#endif
#ifdef mpack_free
    #error "Define MPACK_FREE, not mpack_free."
#endif

// We don't use calloc() at all.
#ifdef MPACK_CALLOC
    #error "Don't define MPACK_CALLOC. MPack does not use calloc()."
#endif
#ifdef mpack_calloc
    #error "Don't define mpack_calloc. MPack does not use calloc()."
#endif

// Use defaults in stdlib if we have them. Without it we don't use malloc.
#if defined(MPACK_STDLIB)
    #if MPACK_STDLIB && !defined(MPACK_MALLOC)
        #define MPACK_MALLOC malloc
        #define MPACK_REALLOC realloc
        #define MPACK_FREE free
    #endif
#endif

/**
 * @}
 */



// This needs to be defined after we've decided whether we have malloc().
#ifndef MPACK_BUILDER
    #if defined(MPACK_MALLOC) && MPACK_WRITER
        #define MPACK_BUILDER 1
    #else
        #define MPACK_BUILDER 0
    #endif
#endif



/**
 * @name System Functions
 * @{
 */

/**
 * @def MPACK_MEMCMP
 *
 * The function MPack will use to provide @c memcmp().
 *
 * Set this to use a custom @c memcmp() function. Your function must have the
 * signature:
 *
 * @code
 * int memcmp(const void* left, const void* right, size_t count);
 * @endcode
 */
/**
 * @def MPACK_MEMCPY
 *
 * The function MPack will use to provide @c memcpy().
 *
 * Set this to use a custom @c memcpy() function. Your function must have the
 * signature:
 *
 * @code
 * void* memcpy(void* restrict dest, const void* restrict src, size_t count);
 * @endcode
 */
/**
 * @def MPACK_MEMMOVE
 *
 * The function MPack will use to provide @c memmove().
 *
 * Set this to use a custom @c memmove() function. Your function must have the
 * signature:
 *
 * @code
 * void* memmove(void* dest, const void* src, size_t count);
 * @endcode
 */
/**
 * @def MPACK_MEMSET
 *
 * The function MPack will use to provide @c memset().
 *
 * Set this to use a custom @c memset() function. Your function must have the
 * signature:
 *
 * @code
 * void* memset(void* p, int c, size_t count);
 * @endcode
 */
/**
 * @def MPACK_STRLEN
 *
 * The function MPack will use to provide @c strlen().
 *
 * Set this to use a custom @c strlen() function. Your function must have the
 * signature:
 *
 * @code
 * size_t strlen(const char* str);
 * @endcode
 */

// These were briefly configurable in lowercase in an unreleased version. Just
// to make sure no one is doing this, we make sure these aren't already defined.
#ifdef mpack_memcmp
    #error "Define MPACK_MEMCMP, not mpack_memcmp."
#endif
#ifdef mpack_memcpy
    #error "Define MPACK_MEMCPY, not mpack_memcpy."
#endif
#ifdef mpack_memmove
    #error "Define MPACK_MEMMOVE, not mpack_memmove."
#endif
#ifdef mpack_memset
    #error "Define MPACK_MEMSET, not mpack_memset."
#endif
#ifdef mpack_strlen
    #error "Define MPACK_STRLEN, not mpack_strlen."
#endif

// If the standard library is available, we prefer to use its functions.
#if MPACK_STDLIB
    #ifndef MPACK_MEMCMP
        #define MPACK_MEMCMP memcmp
    #endif
    #ifndef MPACK_MEMCPY
        #define MPACK_MEMCPY memcpy
    #endif
    #ifndef MPACK_MEMMOVE
        #define MPACK_MEMMOVE memmove
    #endif
    #ifndef MPACK_MEMSET
        #define MPACK_MEMSET memset
    #endif
    #ifndef MPACK_STRLEN
        #define MPACK_STRLEN strlen
    #endif
#endif

#if !MPACK_NO_BUILTINS
    #ifdef __has_builtin
        #if !defined(MPACK_MEMCMP) && __has_builtin(__builtin_memcmp)
            #define MPACK_MEMCMP __builtin_memcmp
        #endif
        #if !defined(MPACK_MEMCPY) && __has_builtin(__builtin_memcpy)
            #define MPACK_MEMCPY __builtin_memcpy
        #endif
        #if !defined(MPACK_MEMMOVE) && __has_builtin(__builtin_memmove)
            #define MPACK_MEMMOVE __builtin_memmove
        #endif
        #if !defined(MPACK_MEMSET) && __has_builtin(__builtin_memset)
            #define MPACK_MEMSET __builtin_memset
        #endif
        #if !defined(MPACK_STRLEN) && __has_builtin(__builtin_strlen)
            #define MPACK_STRLEN __builtin_strlen
        #endif
    #elif defined(__GNUC__)
        #ifndef MPACK_MEMCMP
            #define MPACK_MEMCMP __builtin_memcmp
        #endif
        #ifndef MPACK_MEMCPY
            #define MPACK_MEMCPY __builtin_memcpy
        #endif
        // There's not always a builtin memmove for GCC. If we can't test for
        // it with __has_builtin above, we don't use it. It's been around for
        // much longer under clang, but then so has __has_builtin, so we let
        // the block above handle it.
        #ifndef MPACK_MEMSET
            #define MPACK_MEMSET __builtin_memset
        #endif
        #ifndef MPACK_STRLEN
            #define MPACK_STRLEN __builtin_strlen
        #endif
    #endif
#endif

/**
 * @}
 */



/**
 * @name Debugging Options
 * @{
 */

/**
 * @def MPACK_DEBUG
 *
 * Enables debug features. You may want to wrap this around your
 * own debug preprocs. By default, this is enabled if @c DEBUG or @c _DEBUG
 * are defined. (@c NDEBUG is not used since it is allowed to have
 * different values in different translation units.)
 */
#if !defined(MPACK_DEBUG)
    #if defined(DEBUG) || defined(_DEBUG)
        #define MPACK_DEBUG 1
    #else
        #define MPACK_DEBUG 0
    #endif
#endif

/**
 * @def MPACK_STRINGS
 *
 * Enables descriptive error and type strings.
 *
 * This can be turned off (by defining it to 0) to maximize space savings
 * on embedded devices. If this is disabled, string functions such as
 * mpack_error_to_string() and mpack_type_to_string() return an empty string.
 */
#ifndef MPACK_STRINGS
    #ifdef __AVR__
        #define MPACK_STRINGS 0
    #else
        #define MPACK_STRINGS 1
    #endif
#endif

/**
 * Set this to 1 to implement a custom @ref mpack_assert_fail() function.
 * See the documentation on @ref mpack_assert_fail() for details.
 *
 * Asserts are only used when @ref MPACK_DEBUG is enabled, and can be
 * triggered by bugs in MPack or bugs due to incorrect usage of MPack.
 */
#ifndef MPACK_CUSTOM_ASSERT
#define MPACK_CUSTOM_ASSERT 0
#endif

/**
 * @def MPACK_READ_TRACKING
 *
 * Enables compound type size tracking for readers. This ensures that the
 * correct number of elements or bytes are read from a compound type.
 *
 * This is enabled by default in debug builds (provided a @c malloc() is
 * available.)
 */
#if !defined(MPACK_READ_TRACKING)
    #if MPACK_DEBUG && MPACK_READER && defined(MPACK_MALLOC)
        #define MPACK_READ_TRACKING 1
    #else
        #define MPACK_READ_TRACKING 0
    #endif
#endif
#if MPACK_READ_TRACKING && !MPACK_READER
    #error "MPACK_READ_TRACKING requires MPACK_READER."
#endif

/**
 * @def MPACK_WRITE_TRACKING
 *
 * Enables compound type size tracking for writers. This ensures that the
 * correct number of elements or bytes are written in a compound type.
 *
 * Note that without write tracking enabled, it is possible for buggy code
 * to emit invalid MessagePack without flagging an error by writing the wrong
 * number of elements or bytes in a compound type. With tracking enabled,
 * MPack will catch such errors and break on the offending line of code.
 *
 * This is enabled by default in debug builds (provided a @c malloc() is
 * available.)
 */
#if !defined(MPACK_WRITE_TRACKING)
    #if MPACK_DEBUG && MPACK_WRITER && defined(MPACK_MALLOC)
        #define MPACK_WRITE_TRACKING 1
    #else
        #define MPACK_WRITE_TRACKING 0
    #endif
#endif
#if MPACK_WRITE_TRACKING && !MPACK_WRITER
    #error "MPACK_WRITE_TRACKING requires MPACK_WRITER."
#endif

/**
 * @}
 */




/**
 * @name Miscellaneous Options
 * @{
 */

/**
 * Whether to optimize for size or speed.
 *
 * Optimizing for size simplifies some parsing and encoding algorithms
 * at the expense of speed and saves a few kilobytes of space in the
 * resulting executable.
 *
 * This automatically detects -Os with GCC/Clang. Unfortunately there
 * doesn't seem to be a macro defined for /Os under MSVC.
 */
#ifndef MPACK_OPTIMIZE_FOR_SIZE
    #ifdef __OPTIMIZE_SIZE__
        #define MPACK_OPTIMIZE_FOR_SIZE 1
    #else
        #define MPACK_OPTIMIZE_FOR_SIZE 0
    #endif
#endif

/**
 * Stack space in bytes to use when initializing a reader or writer
 * with a stack-allocated buffer.
 *
 * @warning Make sure you have sufficient stack space. Some libc use relatively
 * small stacks even on desktop platforms, e.g. musl.
 */
#ifndef MPACK_STACK_SIZE
#define MPACK_STACK_SIZE 4096
#endif

/**
 * Buffer size to use for allocated buffers (such as for a file writer.)
 *
 * Starting with a single page and growing as needed seems to
 * provide the best performance with minimal memory waste.
 * Increasing this does not improve performance even when writing
 * huge messages.
 */
#ifndef MPACK_BUFFER_SIZE
#define MPACK_BUFFER_SIZE 4096
#endif

/**
 * Minimum size for paged allocations in bytes.
 *
 * This is the value used by default for MPACK_NODE_PAGE_SIZE and
 * MPACK_BUILDER_PAGE_SIZE.
 */
#ifndef MPACK_PAGE_SIZE
#define MPACK_PAGE_SIZE 4096
#endif

/**
 * Minimum size of an allocated node page in bytes.
 *
 * The children for a given compound element must be contiguous, so
 * larger pages than this may be allocated as needed. (Safety checks
 * exist to prevent malicious data from causing too large allocations.)
 *
 * See @ref mpack_node_data_t for the size of nodes.
 *
 * Using as many nodes fit in one memory page seems to provide the
 * best performance, and has very little waste when parsing small
 * messages.
 */
#ifndef MPACK_NODE_PAGE_SIZE
#define MPACK_NODE_PAGE_SIZE MPACK_PAGE_SIZE
#endif

/**
 * Minimum size of an allocated builder page in bytes.
 *
 * Builder writes are deferred to the allocated builder buffer which is
 * composed of a list of buffer pages. This defines the size of those pages.
 */
#ifndef MPACK_BUILDER_PAGE_SIZE
#define MPACK_BUILDER_PAGE_SIZE MPACK_PAGE_SIZE
#endif

/**
 * @def MPACK_BUILDER_INTERNAL_STORAGE
 *
 * Enables a small amount of internal storage within the writer to avoid some
 * allocations when using builders.
 *
 * This is disabled by default. Enable it to potentially improve performance at
 * the expense of a larger writer.
 *
 * @see MPACK_BUILDER_INTERNAL_STORAGE_SIZE to configure its size.
 */
#ifndef MPACK_BUILDER_INTERNAL_STORAGE
#define MPACK_BUILDER_INTERNAL_STORAGE 0
#endif

/**
 * Amount of space reserved inside @ref mpack_writer_t for the Builders. This
 * can allow small messages to be built with the Builder API without incurring
 * an allocation.
 *
 * Builder metadata is placed in this space in addition to the literal
 * MessagePack data. It needs to be big enough to be useful, but not so big as
 * to overflow the stack. If more space is needed, pages are allocated.
 *
 * This is only used if MPACK_BUILDER_INTERNAL_STORAGE is enabled.
 *
 * @see MPACK_BUILDER_PAGE_SIZE
 * @see MPACK_BUILDER_INTERNAL_STORAGE
 *
 * @warning Writers are typically placed on the stack so make sure you have
 * sufficient stack space. Some libc use relatively small stacks even on
 * desktop platforms, e.g. musl.
 */
#ifndef MPACK_BUILDER_INTERNAL_STORAGE_SIZE
#define MPACK_BUILDER_INTERNAL_STORAGE_SIZE 256
#endif

/**
 * The initial depth for the node parser. When MPACK_MALLOC is available,
 * the node parser has no practical depth limit, and it is not recursive
 * so there is no risk of overflowing the call stack.
 */
#ifndef MPACK_NODE_INITIAL_DEPTH
#define MPACK_NODE_INITIAL_DEPTH 8
#endif

/**
 * The maximum depth for the node parser if @ref MPACK_MALLOC is not available.
 */
#ifndef MPACK_NODE_MAX_DEPTH_WITHOUT_MALLOC
#define MPACK_NODE_MAX_DEPTH_WITHOUT_MALLOC 32
#endif

/**
 * @def MPACK_NO_BUILTINS
 *
 * Whether to disable compiler intrinsics and other built-in functions.
 *
 * If this is enabled, MPack won't use `__attribute__`, `__declspec`, any
 * function starting with `__builtin`, or pretty much anything else that isn't
 * standard C.
 */
#if defined(MPACK_DOXYGEN)
#if MPACK_DOXYGEN
    #define MPACK_NO_BUILTINS 0
#endif
#endif

/**
 * @}
 */



#if MPACK_DEBUG
/**
 * @name Debug Functions
 * @{
 */
/**
 * Implement this and define @ref MPACK_CUSTOM_ASSERT to use a custom
 * assertion function.
 *
 * This function should not return. If it does, MPack will @c abort().
 *
 * If you use C++, make sure you include @c mpack.h where you define
 * this to get the correct linkage (or define it <code>extern "C"</code>.)
 *
 * Asserts are only used when @ref MPACK_DEBUG is enabled, and can be
 * triggered by bugs in MPack or bugs due to incorrect usage of MPack.
 */
void mpack_assert_fail(const char* message);
/**
 * @}
 */
#endif



// The rest of this file shouldn't show up in Doxygen docs.
/** @cond */



/*
 * All remaining pseudo-configuration options that have not yet been set must
 * be defined here in order to support -Wundef.
 *
 * These aren't real configuration options; they are implementation details of
 * MPack.
 */
#ifndef MPACK_CUSTOM_BREAK
#define MPACK_CUSTOM_BREAK 0
#endif
#ifndef MPACK_EMIT_INLINE_DEFS
#define MPACK_EMIT_INLINE_DEFS 0
#endif
#ifndef MPACK_AMALGAMATED
#define MPACK_AMALGAMATED 0
#endif
#ifndef MPACK_RELEASE_VERSION
#define MPACK_RELEASE_VERSION 0
#endif
#ifndef MPACK_INTERNAL
#define MPACK_INTERNAL 0
#endif



/* System headers (based on configuration) */

#if MPACK_CONFORMING
    #include <stddef.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <inttypes.h>
    #include <limits.h>
#endif

#if MPACK_STDLIB
    #include <string.h>
    #include <stdlib.h>
#endif

#if MPACK_STDIO
    #include <stdio.h>
    #include <errno.h>
    #if MPACK_DEBUG
        #include <stdarg.h>
    #endif
#endif



/*
 * Integer Constants and Limits
 */

#if MPACK_CONFORMING
    #define MPACK_INT64_C INT64_C
    #define MPACK_UINT64_C UINT64_C

    #define MPACK_INT8_MIN INT8_MIN
    #define MPACK_INT16_MIN INT16_MIN
    #define MPACK_INT32_MIN INT32_MIN
    #define MPACK_INT64_MIN INT64_MIN
    #define MPACK_INT_MIN INT_MIN

    #define MPACK_INT8_MAX INT8_MAX
    #define MPACK_INT16_MAX INT16_MAX
    #define MPACK_INT32_MAX INT32_MAX
    #define MPACK_INT64_MAX INT64_MAX
    #define MPACK_INT_MAX INT_MAX

    #define MPACK_UINT8_MAX UINT8_MAX
    #define MPACK_UINT16_MAX UINT16_MAX
    #define MPACK_UINT32_MAX UINT32_MAX
    #define MPACK_UINT64_MAX UINT64_MAX
    #define MPACK_UINT_MAX UINT_MAX
#else
    // For a non-conforming implementation we assume int is 32 bits.

    #define MPACK_INT64_C(x) ((int64_t)(x##LL))
    #define MPACK_UINT64_C(x) ((uint64_t)(x##LLU))

    #define MPACK_INT8_MIN ((int8_t)(0x80))
    #define MPACK_INT16_MIN ((int16_t)(0x8000))
    #define MPACK_INT32_MIN ((int32_t)(0x80000000))
    #define MPACK_INT64_MIN MPACK_INT64_C(0x8000000000000000)
    #define MPACK_INT_MIN MPACK_INT32_MIN

    #define MPACK_INT8_MAX ((int8_t)(0x7f))
    #define MPACK_INT16_MAX ((int16_t)(0x7fff))
    #define MPACK_INT32_MAX ((int32_t)(0x7fffffff))
    #define MPACK_INT64_MAX MPACK_INT64_C(0x7fffffffffffffff)
    #define MPACK_INT_MAX MPACK_INT32_MAX

    #define MPACK_UINT8_MAX ((uint8_t)(0xffu))
    #define MPACK_UINT16_MAX ((uint16_t)(0xffffu))
    #define MPACK_UINT32_MAX ((uint32_t)(0xffffffffu))
    #define MPACK_UINT64_MAX MPACK_UINT64_C(0xffffffffffffffff)
    #define MPACK_UINT_MAX MPACK_UINT32_MAX
#endif



/*
 * Floating point support
 */

#if MPACK_DOUBLE && !MPACK_FLOAT
    #error "MPACK_DOUBLE requires MPACK_FLOAT."
#endif

// If we don't have support for float or double, we poison the identifiers to
// make sure we don't define anything related to them.
#if MPACK_INTERNAL && defined(MPACK_UNIT_TESTS) && defined(__GNUC__)
    #if !MPACK_FLOAT
        #pragma GCC poison float
    #endif
    #if !MPACK_DOUBLE
        #pragma GCC poison double
    #endif
#endif



/*
 * extern C
 */

#ifdef __cplusplus
    #define MPACK_EXTERN_C_BEGIN extern "C" {
    #define MPACK_EXTERN_C_END   }
#else
    #define MPACK_EXTERN_C_BEGIN /*nothing*/
    #define MPACK_EXTERN_C_END   /*nothing*/
#endif



/*
 * Warnings
 */

#if defined(__GNUC__)
    // Diagnostic push is not supported before GCC 4.6.
    #if defined(__clang__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
        #define MPACK_SILENCE_WARNINGS_PUSH _Pragma ("GCC diagnostic push")
        #define MPACK_SILENCE_WARNINGS_POP _Pragma ("GCC diagnostic pop")
    #endif
#elif defined(_MSC_VER)
    // To support VS2017 and earlier we need to use __pragma and not _Pragma
    #define MPACK_SILENCE_WARNINGS_PUSH __pragma(warning(push))
    #define MPACK_SILENCE_WARNINGS_POP __pragma(warning(pop))
#endif

#if defined(_MSC_VER)
    // These are a bunch of mostly useless warnings emitted under MSVC /W4,
    // some as a result of the expansion of macros.
    #define MPACK_SILENCE_WARNINGS_MSVC_W4 \
            __pragma(warning(disable:4996)) /* _CRT_SECURE_NO_WARNINGS */ \
            __pragma(warning(disable:4127)) /* comparison is constant */ \
            __pragma(warning(disable:4702)) /* unreachable code */ \
            __pragma(warning(disable:4310)) /* cast truncates constant value */
#else
    #define MPACK_SILENCE_WARNINGS_MSVC_W4 /*nothing*/
#endif

/* GCC versions before 5.1 warn about defining a C99 non-static inline function
 * before declaring it (see issue #20). */
#if defined(__GNUC__) && !defined(__clang__)
    #if __GNUC__ < 5 || (__GNUC__ == 5 && __GNUC_MINOR__ < 1)
        #ifdef __cplusplus
            #define MPACK_SILENCE_WARNINGS_MISSING_PROTOTYPES \
                _Pragma ("GCC diagnostic ignored \"-Wmissing-declarations\"")
        #else
            #define MPACK_SILENCE_WARNINGS_MISSING_PROTOTYPES \
                _Pragma ("GCC diagnostic ignored \"-Wmissing-prototypes\"")
        #endif
    #endif
#endif
#ifndef MPACK_SILENCE_WARNINGS_MISSING_PROTOTYPES
    #define MPACK_SILENCE_WARNINGS_MISSING_PROTOTYPES /*nothing*/
#endif

/* GCC versions before 4.8 warn about shadowing a function with a variable that
 * isn't a function or function pointer (like "index"). */
#if defined(__GNUC__) && !defined(__clang__)
    #if __GNUC__ == 4 && __GNUC_MINOR__ < 8
        #define MPACK_SILENCE_WARNINGS_SHADOW \
            _Pragma ("GCC diagnostic ignored \"-Wshadow\"")
    #endif
#endif
#ifndef MPACK_SILENCE_WARNINGS_SHADOW
    #define MPACK_SILENCE_WARNINGS_SHADOW /*nothing*/
#endif

// On platforms with small size_t (e.g. AVR) we get type limits warnings where
// we compare a size_t to e.g. MPACK_UINT32_MAX.
#ifdef __AVR__
    #define MPACK_SILENCE_WARNINGS_TYPE_LIMITS \
        _Pragma ("GCC diagnostic ignored \"-Wtype-limits\"")
#else
    #define MPACK_SILENCE_WARNINGS_TYPE_LIMITS /*nothing*/
#endif

// MPack uses declarations after statements. This silences warnings about it
// (e.g. when using MPack in a Linux kernel module.)
#if defined(__GNUC__) && !defined(__cplusplus)
    #define MPACK_SILENCE_WARNINGS_DECLARATION_AFTER_STATEMENT \
        _Pragma ("GCC diagnostic ignored \"-Wdeclaration-after-statement\"")
#else
    #define MPACK_SILENCE_WARNINGS_DECLARATION_AFTER_STATEMENT /*nothing*/
#endif

#ifdef MPACK_SILENCE_WARNINGS_PUSH
    // We only silence warnings if push/pop is supported, that way we aren't
    // silencing warnings in code that uses MPack. If your compiler doesn't
    // support push/pop silencing of warnings, you'll have to turn off
    // conflicting warnings manually.

    #define MPACK_SILENCE_WARNINGS_BEGIN \
        MPACK_SILENCE_WARNINGS_PUSH \
        MPACK_SILENCE_WARNINGS_MSVC_W4 \
        MPACK_SILENCE_WARNINGS_MISSING_PROTOTYPES \
        MPACK_SILENCE_WARNINGS_SHADOW \
        MPACK_SILENCE_WARNINGS_TYPE_LIMITS \
        MPACK_SILENCE_WARNINGS_DECLARATION_AFTER_STATEMENT

    #define MPACK_SILENCE_WARNINGS_END \
        MPACK_SILENCE_WARNINGS_POP
#else
    #define MPACK_SILENCE_WARNINGS_BEGIN /*nothing*/
    #define MPACK_SILENCE_WARNINGS_END /*nothing*/
#endif

MPACK_SILENCE_WARNINGS_BEGIN
MPACK_EXTERN_C_BEGIN



/* Miscellaneous helper macros */

#define MPACK_UNUSED(var) ((void)(var))

#define MPACK_STRINGIFY_IMPL(arg) #arg
#define MPACK_STRINGIFY(arg) MPACK_STRINGIFY_IMPL(arg)

// This is a workaround for MSVC's incorrect expansion of __VA_ARGS__. It
// treats __VA_ARGS__ as a single preprocessor token when passed in the
// argument list of another macro unless we use an outer wrapper to expand it
// lexically first. (For safety/consistency we use this in all variadic macros
// that don't ignore the variadic arguments regardless of whether __VA_ARGS__
// is passed to another macro.)
//     https://stackoverflow.com/a/32400131
#define MPACK_EXPAND(x) x

// Extracts the first argument of a variadic macro list, where there might only
// be one argument.
#define MPACK_EXTRACT_ARG0_IMPL(first, ...) first
#define MPACK_EXTRACT_ARG0(...) MPACK_EXPAND(MPACK_EXTRACT_ARG0_IMPL( __VA_ARGS__ , ignored))

// Stringifies the first argument of a variadic macro list, where there might
// only be one argument.
#define MPACK_STRINGIFY_ARG0_impl(first, ...) #first
#define MPACK_STRINGIFY_ARG0(...) MPACK_EXPAND(MPACK_STRINGIFY_ARG0_impl( __VA_ARGS__ , ignored))



/*
 * Definition of inline macros.
 *
 * MPack does not use static inline in header files; only one non-inline definition
 * of each function should exist in the final build. This can reduce the binary size
 * in cases where the compiler cannot or chooses not to inline a function.
 * The addresses of functions should also compare equal across translation units
 * regardless of whether they are declared inline.
 *
 * The above requirements mean that the declaration and definition of non-trivial
 * inline functions must be separated so that the definitions will only
 * appear when necessary. In addition, three different linkage models need
 * to be supported:
 *
 *  - The C99 model, where a standalone function is emitted only if there is any
 *    `extern inline` or non-`inline` declaration (including the definition itself)
 *
 *  - The GNU model, where an `inline` definition emits a standalone function and an
 *    `extern inline` definition does not, regardless of other declarations
 *
 *  - The C++ model, where `inline` emits a standalone function with special
 *    (COMDAT) linkage
 *
 * The macros below wrap up everything above. All inline functions defined in header
 * files have a single non-inline definition emitted in the compilation of
 * mpack-platform.c. All inline declarations and definitions use the same MPACK_INLINE
 * specification to simplify the rules on when standalone functions are emitted.
 * Inline functions in source files are defined MPACK_STATIC_INLINE.
 *
 * Additional reading:
 *     http://www.greenend.org.uk/rjk/tech/inline.html
 */

#if defined(__cplusplus)
    // C++ rules
    // The linker will need COMDAT support to link C++ object files,
    // so we don't need to worry about emitting definitions from C++
    // translation units. If mpack-platform.c (or the amalgamation)
    // is compiled as C, its definition will be used, otherwise a
    // C++ definition will be used, and no other C files will emit
    // a definition.
    #define MPACK_INLINE inline

#elif defined(_MSC_VER)
    // MSVC 2013 always uses COMDAT linkage, but it doesn't treat 'inline' as a
    // keyword in C99 mode. (This appears to be fixed in a later version of
    // MSVC but we don't bother detecting it.)
    #define MPACK_INLINE __inline
    #define MPACK_STATIC_INLINE static __inline

#elif defined(__GNUC__) && (defined(__GNUC_GNU_INLINE__) || \
        (!defined(__GNUC_STDC_INLINE__) && !defined(__GNUC_GNU_INLINE__)))
    // GNU rules
    #if MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE inline
    #else
        #define MPACK_INLINE extern inline
    #endif

#elif defined(__TINYC__)
    // tcc ignores the inline keyword, so we have to use static inline. We
    // issue a warning to make sure you are aware. You can define the below
    // macro to disable the warning. Hopefully this will be fixed soon:
    //     https://lists.nongnu.org/archive/html/tinycc-devel/2019-06/msg00000.html
    #ifndef MPACK_DISABLE_TINYC_INLINE_WARNING
        #warning "Single-definition inline is not supported by tcc. All inlines will be static. Define MPACK_DISABLE_TINYC_INLINE_WARNING to disable this warning."
    #endif
    #define MPACK_INLINE static inline

#else
    // C99 rules
    #if MPACK_EMIT_INLINE_DEFS
        #define MPACK_INLINE extern inline
    #else
        #define MPACK_INLINE inline
    #endif
#endif

#ifndef MPACK_STATIC_INLINE
#define MPACK_STATIC_INLINE static inline
#endif

#ifdef MPACK_OPTIMIZE_FOR_SPEED
    #error "You should define MPACK_OPTIMIZE_FOR_SIZE, not MPACK_OPTIMIZE_FOR_SPEED."
#endif



/*
 * Prevent inlining
 *
 * When a function is only used once, it is almost always inlined
 * automatically. This can cause poor instruction cache usage because a
 * function that should rarely be called (such as buffer exhaustion handling)
 * will get inlined into the middle of a hot code path.
 */

#if !MPACK_NO_BUILTINS
    #if defined(_MSC_VER)
        #define MPACK_NOINLINE __declspec(noinline)
    #elif defined(__GNUC__) || defined(__clang__)
        #define MPACK_NOINLINE __attribute__((__noinline__))
    #endif
#endif
#ifndef MPACK_NOINLINE
    #define MPACK_NOINLINE /* nothing */
#endif



/* restrict */

// We prefer the builtins even though e.g. MSVC's __restrict may not have
// exactly the same behaviour as the proper C99 restrict keyword because the
// builtins work in C++, so using the same keyword in both C and C++ prevents
// any incompatibilities when using MPack compiled as C in C++ code.
#if !MPACK_NO_BUILTINS
    #if defined(__GNUC__)
        #define MPACK_RESTRICT __restrict__
    #elif defined(_MSC_VER)
        #define MPACK_RESTRICT __restrict
    #endif
#endif

#ifndef MPACK_RESTRICT
    #ifdef __cplusplus
        #define MPACK_RESTRICT /* nothing, unavailable in C++ */
    #endif
#endif

#ifndef MPACK_RESTRICT
    #ifdef _MSC_VER
        // MSVC 2015 apparently doesn't properly support the restrict keyword
        // in C. We're using builtins above which do work on 2015, but when
        // MPACK_NO_BUILTINS is enabled we can't use it.
        #if _MSC_VER < 1910
            #define MPACK_RESTRICT /*nothing*/
        #endif
    #endif
#endif

#ifndef MPACK_RESTRICT
    #define MPACK_RESTRICT restrict /* required in C99 */
#endif



/* Some compiler-specific keywords and builtins */

#if !MPACK_NO_BUILTINS
    #if defined(__GNUC__) || defined(__clang__)
        #define MPACK_UNREACHABLE __builtin_unreachable()
        #define MPACK_NORETURN(fn) fn __attribute__((__noreturn__))
    #elif defined(_MSC_VER)
        #define MPACK_UNREACHABLE __assume(0)
        #define MPACK_NORETURN(fn) __declspec(noreturn) fn
    #endif
#endif

#ifndef MPACK_UNREACHABLE
#define MPACK_UNREACHABLE ((void)0)
#endif
#ifndef MPACK_NORETURN
#define MPACK_NORETURN(fn) fn
#endif



/*
 * Likely/unlikely
 *
 * These should only really be used when a branch is taken (or not taken) less
 * than 1/1000th of the time. Buffer flush checks when writing very small
 * elements are a good example.
 */

#if !MPACK_NO_BUILTINS
    #if defined(__GNUC__) || defined(__clang__)
        #ifndef MPACK_LIKELY
            #define MPACK_LIKELY(x) __builtin_expect((x),1)
        #endif
        #ifndef MPACK_UNLIKELY
            #define MPACK_UNLIKELY(x) __builtin_expect((x),0)
        #endif
    #endif
#endif

#ifndef MPACK_LIKELY
    #define MPACK_LIKELY(x) (x)
#endif
#ifndef MPACK_UNLIKELY
    #define MPACK_UNLIKELY(x) (x)
#endif



/* alignof */

#ifndef MPACK_ALIGNOF
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #define MPACK_ALIGNOF(T) (_Alignof(T))
        #endif
    #endif
#endif

#ifndef MPACK_ALIGNOF
    #if defined(__cplusplus)
        #if __cplusplus >= 201103L
            #define MPACK_ALIGNOF(T) (alignof(T))
        #endif
    #endif
#endif

#ifndef MPACK_ALIGNOF
    #if defined(__GNUC__) && !defined(MPACK_NO_BUILTINS)
        #if defined(__clang__) || __GNUC__ >= 4
            #define MPACK_ALIGNOF(T) (__alignof__(T))
        #endif
    #endif
#endif

#ifndef MPACK_ALIGNOF
    #ifdef _MSC_VER
        #define MPACK_ALIGNOF(T) __alignof(T)
    #endif
#endif

// MPACK_ALIGNOF may not exist, in which case a workaround is used.



/* Static assert */

#ifndef MPACK_STATIC_ASSERT
    #if defined(__cplusplus)
        #if __cplusplus >= 201103L
            #define MPACK_STATIC_ASSERT static_assert
        #endif
    #elif defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #define MPACK_STATIC_ASSERT _Static_assert
        #endif
    #endif
#endif

#if !MPACK_NO_BUILTINS
    #ifndef MPACK_STATIC_ASSERT
        #if defined(__has_feature)
            #if __has_feature(cxx_static_assert)
                #define MPACK_STATIC_ASSERT static_assert
            #elif __has_feature(c_static_assert)
                #define MPACK_STATIC_ASSERT _Static_assert
            #endif
        #endif
    #endif

    #ifndef MPACK_STATIC_ASSERT
        #if defined(__GNUC__)
            /* Diagnostic push is not supported before GCC 4.6. */
            #if defined(__clang__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
                #ifndef __cplusplus
                    #if defined(__clang__) || __GNUC__ >= 5
                    #define MPACK_IGNORE_PEDANTIC "GCC diagnostic ignored \"-Wpedantic\""
                    #else
                    #define MPACK_IGNORE_PEDANTIC "GCC diagnostic ignored \"-pedantic\""
                    #endif
                    #define MPACK_STATIC_ASSERT(expr, str) do { \
                        _Pragma ("GCC diagnostic push") \
                        _Pragma (MPACK_IGNORE_PEDANTIC) \
                        _Pragma ("GCC diagnostic ignored \"-Wc++-compat\"") \
                        _Static_assert(expr, str); \
                        _Pragma ("GCC diagnostic pop") \
                    } while (0)
                #endif
            #endif
        #endif
    #endif

    #ifndef MPACK_STATIC_ASSERT
        #ifdef _MSC_VER
            #if _MSC_VER >= 1600
                #define MPACK_STATIC_ASSERT static_assert
            #endif
        #endif
    #endif
#endif

#ifndef MPACK_STATIC_ASSERT
    #define MPACK_STATIC_ASSERT(expr, str) (MPACK_UNUSED(sizeof(char[1 - 2*!(expr)])))
#endif



/* _Generic */

#ifndef MPACK_HAS_GENERIC
    #if defined(__clang__) && defined(__has_feature)
        // With Clang we can test for _Generic support directly
        // and ignore C/C++ version
        #if __has_feature(c_generic_selections)
            #define MPACK_HAS_GENERIC 1
        #else
            #define MPACK_HAS_GENERIC 0
        #endif
    #endif
#endif

#ifndef MPACK_HAS_GENERIC
    #if defined(__STDC_VERSION__)
        #if __STDC_VERSION__ >= 201112L
            #if defined(__GNUC__) && !defined(__clang__)
                // GCC does not have full C11 support in GCC 4.7 and 4.8
                #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
                    #define MPACK_HAS_GENERIC 1
                #endif
            #else
                // We hope other compilers aren't lying about C11/_Generic support
                #define MPACK_HAS_GENERIC 1
            #endif
        #endif
    #endif
#endif

#ifndef MPACK_HAS_GENERIC
    #define MPACK_HAS_GENERIC 0
#endif



/*
 * Finite Math
 *
 * -ffinite-math-only, included in -ffast-math, breaks functions that
 * that check for non-finite real values such as isnan() and isinf().
 *
 * We should use this to trap errors when reading data that contains
 * non-finite reals. This isn't currently implemented.
 */

#ifndef MPACK_FINITE_MATH
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#define MPACK_FINITE_MATH 1
#endif
#endif

#ifndef MPACK_FINITE_MATH
#define MPACK_FINITE_MATH 0
#endif



/*
 * Endianness checks
 *
 * These define MPACK_NHSWAP*() which swap network<->host byte
 * order when needed.
 *
 * We leave them undefined if we can't determine the endianness
 * at compile-time, in which case we fall back to bit-shifts.
 *
 * See the notes in mpack-common.h.
 */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define MPACK_NHSWAP16(x) (x)
        #define MPACK_NHSWAP32(x) (x)
        #define MPACK_NHSWAP64(x) (x)
    #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

        #if !MPACK_NO_BUILTINS
            #if defined(__clang__)
                #ifdef __has_builtin
                    // Unlike the GCC builtins, the bswap builtins in Clang
                    // significantly improve ARM performance.
                    #if __has_builtin(__builtin_bswap16)
                        #define MPACK_NHSWAP16(x) __builtin_bswap16(x)
                    #endif
                    #if __has_builtin(__builtin_bswap32)
                        #define MPACK_NHSWAP32(x) __builtin_bswap32(x)
                    #endif
                    #if __has_builtin(__builtin_bswap64)
                        #define MPACK_NHSWAP64(x) __builtin_bswap64(x)
                    #endif
                #endif

            #elif defined(__GNUC__)

                // The GCC bswap builtins are apparently poorly optimized on older
                // versions of GCC, so we set a minimum version here just in case.
                //     http://hardwarebug.org/2010/01/14/beware-the-builtins/

                #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
                    #define MPACK_NHSWAP64(x) __builtin_bswap64(x)
                #endif

                // __builtin_bswap16() was not implemented on all platforms
                // until GCC 4.8.0:
                //     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
                //
                // The 16- and 32-bit versions in GCC significantly reduce performance
                // on ARM with little effect on code size so we don't use them.

            #endif
        #endif
    #endif

#elif defined(_MSC_VER) && defined(_WIN32) && MPACK_STDLIB && !MPACK_NO_BUILTINS

    // On Windows, we assume x86 and x86_64 are always little-endian.
    // We make no assumptions about ARM even though all current
    // Windows Phone devices are little-endian in case Microsoft's
    // compiler is ever used with a big-endian ARM device.

    // These are functions in <stdlib.h> so we depend on MPACK_STDLIB.
    // It's not clear if these are actually faster than just doing the
    // swap manually; maybe we shouldn't bother with this.

    #if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
        #define MPACK_NHSWAP16(x) _byteswap_ushort(x)
        #define MPACK_NHSWAP32(x) _byteswap_ulong(x)
        #define MPACK_NHSWAP64(x) _byteswap_uint64(x)
    #endif

#endif

#if defined(__FLOAT_WORD_ORDER__) && defined(__BYTE_ORDER__)

    // We check where possible that the float byte order matches the
    // integer byte order. This is extremely unlikely to fail, but
    // we check anyway just in case.
    //
    // (The static assert is placed in float/double encoders instead
    // of here because our static assert fallback doesn't work at
    // file scope)

    #define MPACK_CHECK_FLOAT_ORDER() \
        MPACK_STATIC_ASSERT(__FLOAT_WORD_ORDER__ == __BYTE_ORDER__, \
            "float byte order does not match int byte order! float/double " \
            "encoding is not properly implemented on this platform.")

#endif

#ifndef MPACK_CHECK_FLOAT_ORDER
    #define MPACK_CHECK_FLOAT_ORDER() /* nothing */
#endif


/*
 * Here we define mpack_assert() and mpack_break(). They both work like a normal
 * assertion function in debug mode, causing a trap or abort. However, on some platforms
 * you can safely resume execution from mpack_break(), whereas mpack_assert() is
 * always fatal.
 *
 * In release mode, mpack_assert() is converted to an assurance to the compiler
 * that the expression cannot be false (via e.g. __assume() or __builtin_unreachable())
 * to improve optimization where supported. There is thus no point in "safely" handling
 * the case of this being false. Writing mpack_assert(0) rarely makes sense (except
 * possibly as a default handler in a switch) since the compiler will throw away any
 * code after it. If at any time an mpack_assert() is not true, the behaviour is
 * undefined. This also means the expression is evaluated even in release.
 *
 * mpack_break() on the other hand is compiled to nothing in release. It is
 * used in situations where we want to highlight a programming error as early as
 * possible (in the debugger), but we still handle the situation safely if it
 * happens in release to avoid producing incorrect results (such as in
 * MPACK_WRITE_TRACKING.) It does not take an expression to test because it
 * belongs in a safe-handling block after its failing condition has been tested.
 *
 * If stdio is available, we can add a format string describing the error, and
 * on some compilers we can declare it noreturn to get correct results from static
 * analysis tools. Note that the format string and arguments are not evaluated unless
 * the assertion is hit.
 *
 * Note that any arguments to mpack_assert() beyond the first are only evaluated
 * if the expression is false (and are never evaluated in release.)
 *
 * mpack_assert_fail() and mpack_break_hit() are defined separately
 * because assert is noreturn and break isn't. This distinction is very
 * important for static analysis tools to give correct results.
 */

#if MPACK_DEBUG
    MPACK_NORETURN(void mpack_assert_fail_wrapper(const char* message));
    #if MPACK_STDIO
        MPACK_NORETURN(void mpack_assert_fail_format(const char* format, ...));
        #define mpack_assert_fail_at(line, file, exprstr, format, ...) \
                MPACK_EXPAND(mpack_assert_fail_format("mpack assertion failed at " file ":" #line "\n%s\n" format, exprstr, __VA_ARGS__))
    #else
        #define mpack_assert_fail_at(line, file, exprstr, format, ...) \
                mpack_assert_fail_wrapper("mpack assertion failed at " file ":" #line "\n" exprstr "\n")
    #endif

    #define mpack_assert_fail_pos(line, file, exprstr, expr, ...) \
            MPACK_EXPAND(mpack_assert_fail_at(line, file, exprstr, __VA_ARGS__))

    // This contains a workaround to the pedantic C99 requirement of having at
    // least one argument to a variadic macro. The first argument is the
    // boolean expression, the optional second argument (if provided) must be a
    // literal format string, and any additional arguments are the format
    // argument list.
    //
    // Unfortunately this means macros are expanded in the expression before it
    // gets stringified. I haven't found a workaround to this.
    //
    // This adds two unused arguments to the format argument list when a
    // format string is provided, so this would complicate the use of
    // -Wformat and __attribute__((__format__)) on mpack_assert_fail_format()
    // if we ever bothered to implement it.
    #define mpack_assert(...) \
            MPACK_EXPAND(((!(MPACK_EXTRACT_ARG0(__VA_ARGS__))) ? \
                mpack_assert_fail_pos(__LINE__, __FILE__, MPACK_STRINGIFY_ARG0(__VA_ARGS__) , __VA_ARGS__ , "", NULL) : \
                (void)0))

    void mpack_break_hit(const char* message);
    #if MPACK_STDIO
        void mpack_break_hit_format(const char* format, ...);
        #define mpack_break_hit_at(line, file, ...) \
                MPACK_EXPAND(mpack_break_hit_format("mpack breakpoint hit at " file ":" #line "\n" __VA_ARGS__))
    #else
        #define mpack_break_hit_at(line, file, ...) \
                mpack_break_hit("mpack breakpoint hit at " file ":" #line )
    #endif
    #define mpack_break_hit_pos(line, file, ...) MPACK_EXPAND(mpack_break_hit_at(line, file, __VA_ARGS__))
    #define mpack_break(...) MPACK_EXPAND(mpack_break_hit_pos(__LINE__, __FILE__, __VA_ARGS__))
#else
    #define mpack_assert(...) \
            (MPACK_EXPAND((!(MPACK_EXTRACT_ARG0(__VA_ARGS__))) ? \
                (MPACK_UNREACHABLE, (void)0) : \
                (void)0))
    #define mpack_break(...) ((void)0)
#endif



// make sure we don't use the stdlib directly during development
#if MPACK_STDLIB && defined(MPACK_UNIT_TESTS) && MPACK_INTERNAL && defined(__GNUC__)
    #undef memcmp
    #undef memcpy
    #undef memmove
    #undef memset
    #undef strlen
    #undef malloc
    #undef calloc
    #undef realloc
    #undef free
    #pragma GCC poison memcmp
    #pragma GCC poison memcpy
    #pragma GCC poison memmove
    #pragma GCC poison memset
    #pragma GCC poison strlen
    #pragma GCC poison malloc
    #pragma GCC poison calloc
    #pragma GCC poison realloc
    #pragma GCC poison free
#endif



// If we don't have these stdlib functions, we need to define them ourselves.
// Either way we give them a lowercase name to make the code a bit nicer.

#ifdef MPACK_MEMCMP
    #define mpack_memcmp MPACK_MEMCMP
#else
    int mpack_memcmp(const void* s1, const void* s2, size_t n);
#endif

#ifdef MPACK_MEMCPY
    #define mpack_memcpy MPACK_MEMCPY
#else
    void* mpack_memcpy(void* MPACK_RESTRICT s1, const void* MPACK_RESTRICT s2, size_t n);
#endif

#ifdef MPACK_MEMMOVE
    #define mpack_memmove MPACK_MEMMOVE
#else
    void* mpack_memmove(void* s1, const void* s2, size_t n);
#endif

#ifdef MPACK_MEMSET
    #define mpack_memset MPACK_MEMSET
#else
    void* mpack_memset(void* s, int c, size_t n);
#endif

#ifdef MPACK_STRLEN
    #define mpack_strlen MPACK_STRLEN
#else
    size_t mpack_strlen(const char* s);
#endif



#if MPACK_STDIO
    #if defined(WIN32)
        #define mpack_snprintf _snprintf
    #else
        #define mpack_snprintf snprintf
    #endif
#endif



/* Debug logging */
#if 0
    #include <stdio.h>
    #define mpack_log(...) (MPACK_EXPAND(printf(__VA_ARGS__)), fflush(stdout))
#else
    #define mpack_log(...) ((void)0)
#endif



/* Make sure our configuration makes sense */
#ifndef MPACK_MALLOC
    #if MPACK_STDIO
        #error "MPACK_STDIO requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #if MPACK_READ_TRACKING
        #error "MPACK_READ_TRACKING requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
    #if MPACK_WRITE_TRACKING
        #error "MPACK_WRITE_TRACKING requires preprocessor definitions for MPACK_MALLOC and MPACK_FREE."
    #endif
#endif



/* Implement realloc if unavailable */
#ifdef MPACK_MALLOC
    #ifdef MPACK_REALLOC
        MPACK_INLINE void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size) {
            MPACK_UNUSED(used_size);
            return MPACK_REALLOC(old_ptr, new_size);
        }
    #else
        void* mpack_realloc(void* old_ptr, size_t used_size, size_t new_size);
    #endif
#endif



/** @endcond */
/**
 * @}
 */

MPACK_EXTERN_C_END
MPACK_SILENCE_WARNINGS_END

#endif
