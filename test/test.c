/*
 * Copyright (c) 2015 Nicholas Fraser
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

#include "test.h"

#include <string.h>
#include <stdarg.h>

#include "test-reader.h"
#include "test-expect.h"
#include "test-write.h"
#include "test-buffer.h"
#include "test-common.h"
#include "test-node.h"
#include "test-file.h"

mpack_tag_t (*fn_mpack_tag_nil)(void) = &mpack_tag_nil;

int passes;
int tests;

#if MPACK_CUSTOM_ASSERT
bool test_jmp_set = false;
jmp_buf test_jmp_buf;
bool test_break_set = false;
bool test_break_hit;

void mpack_assert_fail(const char* message) {
    if (!test_jmp_set) {
        TEST_TRUE(false, "assertion hit! %s", message);
        abort();
    }
    longjmp(test_jmp_buf, 1);
}

void mpack_break_hit(const char* message) {
    if (!test_break_set) {
        TEST_TRUE(false, "break hit! %s", message);
        abort();
    }
    test_break_hit = true;
}
#endif

void test_true_impl(bool result, const char* file, int line, const char* format, ...) {
    ++tests;
    if (result) {
        ++passes;
    } else {
        printf("TEST FAILED AT %s:%i --", file, line);

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        printf("\n");
        if (TEST_EARLY_EXIT)
            abort();
    }
}

int main(void) {
    printf("\n\n");

    test_common();

    #if MPACK_READER
    test_reader();
    #endif
    #if MPACK_EXPECT
    test_expect();
    #endif
    #if MPACK_WRITER
    test_writes();
    #endif
    #if MPACK_NODE
    test_node();
    #endif
    #if MPACK_STDIO
    test_file();
    #endif

    test_buffers();

    printf("\n\nUnit testing complete. %i failures in %i checks.\n\n\n", tests - passes, tests);
    return (passes == tests) ? EXIT_SUCCESS : EXIT_FAILURE;
}

