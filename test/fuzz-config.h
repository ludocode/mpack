#ifndef MPACK_FUZZ_CONFIG_H
#define MPACK_FUZZ_CONFIG_H

// we use small buffer sizes to test flushing and growing
#define MPACK_TRACKING_INITIAL_CAPACITY 3
#define MPACK_STACK_SIZE 33
#define MPACK_BUFFER_SIZE 33
#define MPACK_NODE_PAGE_SIZE 113
#define MPACK_NODE_INITIAL_DEPTH 3

#endif
