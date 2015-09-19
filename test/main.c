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

#include "test-expect.h"
#include "test-write.h"
#include "test-buffer.h"
#include "test-tag.h"
#include "test-node.h"
#include "test-file.h"

mpack_tag_t (*fn_mpack_tag_nil)(void) = &mpack_tag_nil;

int passes;
int tests;

int main(void) {
    printf("\n\n");

    test_tags();

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

    printf("\n\nUnit testing complete. %i passes out of %i tests.\n\n\n", passes, tests);
    return (passes == tests) ? EXIT_SUCCESS : EXIT_FAILURE;
}

