/*
 * Copyright (c) 2015-2016 Nicholas Fraser
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

#define MPACK_INTERNAL 1

#include "mpack-node.h"

#if MPACK_NODE



/*
 * Tree Parsing
 */

#ifdef MPACK_MALLOC

// fix up the alloc size to make sure it exactly fits the
// maximum number of nodes it can contain (the allocator will
// waste it back anyway, but we round it down just in case)

#define MPACK_NODES_PER_PAGE \
    ((MPACK_NODE_PAGE_SIZE - sizeof(mpack_tree_page_t)) / sizeof(mpack_node_data_t) + 1)

#define MPACK_PAGE_ALLOC_SIZE \
    (sizeof(mpack_tree_page_t) + sizeof(mpack_node_data_t) * (MPACK_NODES_PER_PAGE - 1))

#endif

typedef struct mpack_level_t {
    mpack_node_data_t* child;
    size_t left; // children left in level
} mpack_level_t;

typedef struct mpack_tree_parser_t {
    mpack_tree_t* tree;
    const char* data;

    // We keep track of the number of "possible nodes" left in the data rather
    // than the number of bytes.
    //
    // When a map or array is parsed, we ensure at least one byte for each child
    // exists and subtract them right away. This ensures that if ever a map or
    // array declares more elements than could possibly be contained in the data,
    // we will error out immediately rather than allocating storage for them.
    //
    // For example malicious data that repeats 0xDE 0xFF 0xFF would otherwise
    // cause us to run out of memory. With this, the parser can only allocate
    // as many nodes as there are bytes in the data (plus the paging overhead,
    // 12%.) An error will be flagged immediately if and when there isn't enough
    // data left to fully read all children of all open compound types on the
    // parsing stack.
    //
    // Once an entire message has been parsed (and there are no nodes left to
    // parse whose bytes have been subtracted), this matches the number of left
    // over bytes in the data.
    size_t possible_nodes_left;

    mpack_node_data_t* nodes; // next node in current page/pool
    size_t nodes_left; // nodes left in current page/pool

    size_t level;
    size_t depth;
    mpack_level_t* stack;
    bool stack_owned;
} mpack_tree_parser_t;

MPACK_STATIC_INLINE uint8_t mpack_tree_u8(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint8_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint8_t val = mpack_load_u8(parser->data);
    parser->data += sizeof(uint8_t);
    parser->possible_nodes_left -= sizeof(uint8_t);
    return val;
}

MPACK_STATIC_INLINE uint16_t mpack_tree_u16(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint16_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint16_t val = mpack_load_u16(parser->data);
    parser->data += sizeof(uint16_t);
    parser->possible_nodes_left -= sizeof(uint16_t);
    return val;
}

MPACK_STATIC_INLINE uint32_t mpack_tree_u32(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint32_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint32_t val = mpack_load_u32(parser->data);
    parser->data += sizeof(uint32_t);
    parser->possible_nodes_left -= sizeof(uint32_t);
    return val;
}

MPACK_STATIC_INLINE uint64_t mpack_tree_u64(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint64_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint64_t val = mpack_load_u64(parser->data);
    parser->data += sizeof(uint64_t);
    parser->possible_nodes_left -= sizeof(uint64_t);
    return val;
}

MPACK_STATIC_INLINE int8_t  mpack_tree_i8 (mpack_tree_parser_t* parser) {return (int8_t) mpack_tree_u8(parser); }
MPACK_STATIC_INLINE int16_t mpack_tree_i16(mpack_tree_parser_t* parser) {return (int16_t)mpack_tree_u16(parser);}
MPACK_STATIC_INLINE int32_t mpack_tree_i32(mpack_tree_parser_t* parser) {return (int32_t)mpack_tree_u32(parser);}
MPACK_STATIC_INLINE int64_t mpack_tree_i64(mpack_tree_parser_t* parser) {return (int64_t)mpack_tree_u64(parser);}

MPACK_STATIC_INLINE void mpack_skip_exttype(mpack_tree_parser_t* parser) {
    // the exttype is stored right before the data. we
    // skip it and get it out of the data when needed.
    mpack_tree_i8(parser);
}

MPACK_STATIC_INLINE float mpack_tree_float(mpack_tree_parser_t* parser) {
    union {
        float f;
        uint32_t i;
    } u;
    u.i = mpack_tree_u32(parser);
    return u.f;
}

MPACK_STATIC_INLINE double mpack_tree_double(mpack_tree_parser_t* parser) {
    union {
        double d;
        uint64_t i;
    } u;
    u.i = mpack_tree_u64(parser);
    return u.d;
}

static void mpack_tree_push_stack(mpack_tree_parser_t* parser, mpack_node_data_t* first_child, size_t total) {

    // No need to push empty containers
    if (total == 0)
        return;

    // Make sure we have enough room in the stack
    if (parser->level + 1 == parser->depth) {
        #ifdef MPACK_MALLOC
        size_t new_depth = parser->depth * 2;
        mpack_log("growing stack to depth %i\n", (int)new_depth);

        // Replace the stack-allocated parsing stack
        if (!parser->stack_owned) {
            mpack_level_t* new_stack = (mpack_level_t*)MPACK_MALLOC(sizeof(mpack_level_t) * new_depth);
            if (!new_stack) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                return;
            }
            mpack_memcpy(new_stack, parser->stack, sizeof(mpack_level_t) * parser->depth);
            parser->stack = new_stack;
            parser->stack_owned = true;

        // Realloc the allocated parsing stack
        } else {
            mpack_level_t* new_stack = (mpack_level_t*)mpack_realloc(parser->stack,
                    sizeof(mpack_level_t) * parser->depth, sizeof(mpack_level_t) * new_depth);
            if (!new_stack) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                return;
            }
            parser->stack = new_stack;
        }
        parser->depth = new_depth;
        #else
        mpack_tree_flag_error(parser->tree, mpack_error_too_big);
        return;
        #endif
    }

    // Push the contents of this node onto the parsing stack
    ++parser->level;
    parser->stack[parser->level].child = first_child;
    parser->stack[parser->level].left = total;
}

static void mpack_tree_parse_children(mpack_tree_parser_t* parser, mpack_node_data_t* node) {
    mpack_type_t type = node->type;
    size_t total = node->len;

    // Calculate total elements to read
    if (type == mpack_type_map) {
        if ((uint64_t)total * 2 > (uint64_t)SIZE_MAX) {
            mpack_tree_flag_error(parser->tree, mpack_error_too_big);
            return;
        }
        total *= 2;
    }

    // Each node is at least one byte. Count these bytes now to make
    // sure there is enough data left.
    if (total > parser->possible_nodes_left) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return;
    }
    parser->possible_nodes_left -= total;

    // If there are enough nodes left in the current page, no need to grow
    if (total <= parser->nodes_left) {
        node->value.children = parser->nodes;
        parser->nodes += total;
        parser->nodes_left -= total;

    } else {

        #ifdef MPACK_MALLOC

        // We can't grow if we're using a fixed pool (i.e. we didn't start with a page)
        if (!parser->tree->next) {
            mpack_tree_flag_error(parser->tree, mpack_error_too_big);
            return;
        }

        // Otherwise we need to grow, and the node's children need to be contiguous.
        // This is a heuristic to decide whether we should waste the remaining space
        // in the current page and start a new one, or give the children their
        // own page. With a fraction of 1/8, this causes at most 12% additional
        // waste. Note that reducing this too much causes less cache coherence and
        // more malloc() overhead due to smaller allocations, so there's a tradeoff
        // here. This heuristic could use some improvement, especially with custom
        // page sizes.

        mpack_tree_page_t* page;

        if (total > MPACK_NODES_PER_PAGE || parser->nodes_left > MPACK_NODES_PER_PAGE / 8) {
            page = (mpack_tree_page_t*)MPACK_MALLOC(
                    sizeof(mpack_tree_page_t) + sizeof(mpack_node_data_t) * (total - 1));
            if (page == NULL) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                return;
            }
            mpack_log("allocated seperate page %p for %i children, %i left in page of %i total\n",
                    page, (int)total, (int)parser->nodes_left, (int)MPACK_NODES_PER_PAGE);

            node->value.children = page->nodes;

        } else {
            page = (mpack_tree_page_t*)MPACK_MALLOC(MPACK_PAGE_ALLOC_SIZE);
            if (page == NULL) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                return;
            }
            mpack_log("allocated new page %p for %i children, wasting %i in page of %i total\n",
                    page, (int)total, (int)parser->nodes_left, (int)MPACK_NODES_PER_PAGE);

            node->value.children = page->nodes;
            parser->nodes = page->nodes + total;
            parser->nodes_left = MPACK_NODES_PER_PAGE - total;
        }

        page->next = parser->tree->next;
        parser->tree->next = page;

        #else
        // We can't grow if we don't have an allocator
        mpack_tree_flag_error(parser->tree, mpack_error_too_big);
        return;
        #endif
    }

    mpack_tree_push_stack(parser, node->value.children, total);
}

static void mpack_tree_parse_bytes(mpack_tree_parser_t* parser, mpack_node_data_t* node) {
    size_t length = node->len;
    if (length > parser->possible_nodes_left) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return;
    }
    node->value.bytes = parser->data;
    parser->data += length;
    parser->possible_nodes_left -= length;
}

static void mpack_tree_parse_node(mpack_tree_parser_t* parser, mpack_node_data_t* node) {
    mpack_assert(node != NULL, "null node?");

    // read the type. we've already accounted for this byte in
    // possible_nodes_left, so we know it is in bounds and don't
    // need to subtract it.
    uint8_t type = mpack_load_u8(parser->data);
    parser->data += sizeof(uint8_t);

    // as with mpack_read_tag(), the fastest way to parse a node is to switch
    // on the first byte, and to explicitly list every possible byte. we switch
    // on the first four bits in size-optimized builds.

    #if MPACK_OPTIMIZE_FOR_SIZE
    switch (type >> 4) {

        // positive fixnum
        case 0x0: case 0x1: case 0x2: case 0x3:
        case 0x4: case 0x5: case 0x6: case 0x7:
            node->type = mpack_type_uint;
            node->value.u = type;
            return;

        // negative fixnum
        case 0xe: case 0xf:
            node->type = mpack_type_int;
            node->value.i = (int8_t)type;
            return;

        // fixmap
        case 0x8:
            node->type = mpack_type_map;
            node->len = (uint32_t)(type & ~0xf0);
            mpack_tree_parse_children(parser, node);
            return;

        // fixarray
        case 0x9:
            node->type = mpack_type_array;
            node->len = (uint32_t)(type & ~0xf0);
            mpack_tree_parse_children(parser, node);
            return;

        // fixstr
        case 0xa: case 0xb:
            node->type = mpack_type_str;
            node->len = (uint32_t)(type & ~0xe0);
            mpack_tree_parse_bytes(parser, node);
            return;

        // not one of the common infix types
        default:
            break;
    }
    #endif

    switch (type) {

        #if !MPACK_OPTIMIZE_FOR_SIZE
        // positive fixnum
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
            node->type = mpack_type_uint;
            node->value.u = type;
            return;

        // negative fixnum
        case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
        case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
            node->type = mpack_type_int;
            node->value.i = (int8_t)type;
            return;

        // fixmap
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
            node->type = mpack_type_map;
            node->len = (uint32_t)(type & ~0xf0);
            mpack_tree_parse_children(parser, node);
            return;

        // fixarray
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            node->type = mpack_type_array;
            node->len = (uint32_t)(type & ~0xf0);
            mpack_tree_parse_children(parser, node);
            return;

        // fixstr
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            node->type = mpack_type_str;
            node->len = (uint32_t)(type & ~0xe0);
            mpack_tree_parse_bytes(parser, node);
            return;
        #endif

        // nil
        case 0xc0:
            node->type = mpack_type_nil;
            return;

        // bool
        case 0xc2: case 0xc3:
            node->type = mpack_type_bool;
            node->value.b = type & 1;
            return;

        // bin8
        case 0xc4:
            node->type = mpack_type_bin;
            node->len = mpack_tree_u8(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // bin16
        case 0xc5:
            node->type = mpack_type_bin;
            node->len = mpack_tree_u16(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // bin32
        case 0xc6:
            node->type = mpack_type_bin;
            node->len = mpack_tree_u32(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // ext8
        case 0xc7:
            node->type = mpack_type_ext;
            node->len = mpack_tree_u8(parser);
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // ext16
        case 0xc8:
            node->type = mpack_type_ext;
            node->len = mpack_tree_u16(parser);
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // ext32
        case 0xc9:
            node->type = mpack_type_ext;
            node->len = mpack_tree_u32(parser);
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // float
        case 0xca:
            node->type = mpack_type_float;
            node->value.f = mpack_tree_float(parser);
            return;

        // double
        case 0xcb:
            node->type = mpack_type_double;
            node->value.d = mpack_tree_double(parser);
            return;

        // uint8
        case 0xcc:
            node->type = mpack_type_uint;
            node->value.u = mpack_tree_u8(parser);
            return;

        // uint16
        case 0xcd:
            node->type = mpack_type_uint;
            node->value.u = mpack_tree_u16(parser);
            return;

        // uint32
        case 0xce:
            node->type = mpack_type_uint;
            node->value.u = mpack_tree_u32(parser);
            return;

        // uint64
        case 0xcf:
            node->type = mpack_type_uint;
            node->value.u = mpack_tree_u64(parser);
            return;

        // int8
        case 0xd0:
            node->type = mpack_type_int;
            node->value.i = mpack_tree_i8(parser);
            return;

        // int16
        case 0xd1:
            node->type = mpack_type_int;
            node->value.i = mpack_tree_i16(parser);
            return;

        // int32
        case 0xd2:
            node->type = mpack_type_int;
            node->value.i = mpack_tree_i32(parser);
            return;

        // int64
        case 0xd3:
            node->type = mpack_type_int;
            node->value.i = mpack_tree_i64(parser);
            return;

        // fixext1
        case 0xd4:
            node->type = mpack_type_ext;
            node->len = 1;
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // fixext2
        case 0xd5:
            node->type = mpack_type_ext;
            node->len = 2;
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // fixext4
        case 0xd6:
            node->type = mpack_type_ext;
            node->len = 4;
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // fixext8
        case 0xd7:
            node->type = mpack_type_ext;
            node->len = 8;
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // fixext16
        case 0xd8:
            node->type = mpack_type_ext;
            node->len = 16;
            mpack_skip_exttype(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // str8
        case 0xd9:
            node->type = mpack_type_str;
            node->len = mpack_tree_u8(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // str16
        case 0xda:
            node->type = mpack_type_str;
            node->len = mpack_tree_u16(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // str32
        case 0xdb:
            node->type = mpack_type_str;
            node->len = mpack_tree_u32(parser);
            mpack_tree_parse_bytes(parser, node);
            return;

        // array16
        case 0xdc:
            node->type = mpack_type_array;
            node->len = mpack_tree_u16(parser);
            mpack_tree_parse_children(parser, node);
            return;

        // array32
        case 0xdd:
            node->type = mpack_type_array;
            node->len = mpack_tree_u32(parser);
            mpack_tree_parse_children(parser, node);
            return;

        // map16
        case 0xde:
            node->type = mpack_type_map;
            node->len = mpack_tree_u16(parser);
            mpack_tree_parse_children(parser, node);
            return;

        // map32
        case 0xdf:
            node->type = mpack_type_map;
            node->len = mpack_tree_u32(parser);
            mpack_tree_parse_children(parser, node);
            return;

        // reserved
        case 0xc1:
            mpack_tree_flag_error(parser->tree, mpack_error_invalid);
            return;

        #if MPACK_OPTIMIZE_FOR_SIZE
        // any other bytes should have been handled by the infix switch
        default:
            mpack_assert(0, "unreachable");
            return;
        #endif
    }

}

static void mpack_tree_parse_elements(mpack_tree_parser_t* parser) {
    mpack_log("parsing tree elements\n");

    // we loop parsing nodes until the parse stack is empty. we break
    // by returning out of the function.
    while (true) {
        mpack_node_data_t* node = parser->stack[parser->level].child;
        --parser->stack[parser->level].left;
        ++parser->stack[parser->level].child;

        mpack_tree_parse_node(parser, node);

        if (mpack_tree_error(parser->tree) != mpack_ok)
            break;

        // pop empty stack levels, exiting the outer loop when the stack is empty.
        // (we could tail-optimize containers by pre-emptively popping empty
        // stack levels before reading the new element, this way we wouldn't
        // have to loop. but we eventually want to use the parse stack to give
        // better error messages that contain the location of the error, so
        // it needs to be complete.)
        while (parser->stack[parser->level].left == 0) {
            if (parser->level == 0)
                return;
            --parser->level;
        }
    }
}

static void mpack_tree_cleanup(mpack_tree_t* tree) {
    MPACK_UNUSED(tree);

    #ifdef MPACK_MALLOC
    mpack_tree_page_t* page = tree->next;
    while (page != NULL) {
        mpack_tree_page_t* next = page->next;
        mpack_log("freeing page %p\n", page);
        MPACK_FREE(page);
        page = next;
    }
    tree->next = NULL;
    #endif
}

static void mpack_tree_parser_setup(mpack_tree_parser_t* parser, mpack_tree_t* tree) {
    mpack_memset(parser, 0, sizeof(*parser));
    parser->tree = tree;
    parser->data = tree->data;
    parser->possible_nodes_left = tree->length;

    #ifdef MPACK_MALLOC
    if (tree->pool == NULL) {

        // allocate first page
        mpack_tree_page_t* page = (mpack_tree_page_t*)MPACK_MALLOC(MPACK_PAGE_ALLOC_SIZE);
        mpack_log("allocated initial page %p of size %i count %i\n",
                page, (int)MPACK_PAGE_ALLOC_SIZE, (int)MPACK_NODES_PER_PAGE);
        if (page == NULL) {
            tree->error = mpack_error_memory;
            return;
        }
        page->next = NULL;
        tree->next = page;

        parser->nodes = page->nodes;
        parser->nodes_left = MPACK_NODES_PER_PAGE;
        tree->root = page->nodes;
        return;
    }
    #endif

    // otherwise use the provided pool
    mpack_assert(tree->pool != NULL, "no pool provided?");
    parser->nodes = tree->pool;
    parser->nodes_left = tree->pool_count;
}

void mpack_tree_parse(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return;
    tree->parsed = true;

    mpack_tree_cleanup(tree);

    mpack_log("starting parse\n");

    if (tree->length == 0) {
        mpack_tree_flag_error(tree, mpack_error_invalid);
        return;
    }

    // setup parser
    mpack_tree_parser_t parser;
    mpack_tree_parser_setup(&parser, tree);
    if (mpack_tree_error(tree) != mpack_ok)
        return;

    // allocate the root node
    tree->root = parser.nodes;
    ++parser.nodes;
    --parser.nodes_left;
    --parser.possible_nodes_left;
    tree->node_count = 1;

    // We read nodes in a loop instead of recursively for maximum
    // performance. The stack holds the amount of children left to
    // read in each level of the tree.

    // Even when we have a malloc() function, it's much faster to
    // allocate the initial parsing stack on the call stack. We
    // replace it with a heap allocation if we need to grow it.
    #ifdef MPACK_MALLOC
    #define MPACK_NODE_STACK_LOCAL_DEPTH MPACK_NODE_INITIAL_DEPTH
    parser.stack_owned = false;
    #else
    #define MPACK_NODE_STACK_LOCAL_DEPTH MPACK_NODE_MAX_DEPTH_WITHOUT_MALLOC
    #endif
    mpack_level_t stack_local[MPACK_NODE_STACK_LOCAL_DEPTH]; // no VLAs in VS 2013
    parser.depth = MPACK_NODE_STACK_LOCAL_DEPTH;
    parser.stack = stack_local;
    #undef MPACK_NODE_STACK_LOCAL_DEPTH
    parser.level = 0;
    parser.stack[0].child = tree->root;
    parser.stack[0].left = 1;

    mpack_tree_parse_elements(&parser);

    #ifdef MPACK_MALLOC
    if (parser.stack_owned)
        MPACK_FREE(parser.stack);
    #endif

    // now that there are no longer any nodes to read, possible_nodes_left
    // is the number of bytes left in the data.
    if (mpack_tree_error(tree) == mpack_ok) {
        tree->size = tree->length - parser.possible_nodes_left;

        // advance past the parsed data
        tree->data += tree->size;
        tree->length -= tree->size;
    }

    mpack_log("parsed tree of %i bytes, %i bytes left\n", (int)tree->size, (int)parser.possible_nodes_left);
    mpack_log("%i nodes in final page\n", (int)parser.nodes_left);
}



/*
 * Tree functions
 */

mpack_node_t mpack_tree_root(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return mpack_tree_nil_node(tree);

    // We check that mpack_tree_parse() was called at least once, and
    // assert if not. This is to facilitation the transition to requiring
    // a call to mpack_tree_parse(), since it used to be automatic on
    // initialization.
    if (!tree->parsed) {
        mpack_break("Tree has not been parsed! You must call mpack_tree_parse()"
                " after initialization before accessing the root node.");
        mpack_tree_flag_error(tree, mpack_error_bug);
        return mpack_tree_nil_node(tree);
    }

    return mpack_node(tree, tree->root);
}

static void mpack_tree_init_clear(mpack_tree_t* tree) {
    mpack_memset(tree, 0, sizeof(*tree));
    tree->nil_node.type = mpack_type_nil;
}

#ifdef MPACK_MALLOC
void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length) {
    mpack_tree_init_clear(tree);

    MPACK_STATIC_ASSERT(MPACK_NODE_PAGE_SIZE >= sizeof(mpack_tree_page_t),
            "MPACK_NODE_PAGE_SIZE is too small");

    MPACK_STATIC_ASSERT(MPACK_PAGE_ALLOC_SIZE <= MPACK_NODE_PAGE_SIZE,
            "incorrect page rounding?");

    tree->data = data;
    tree->length = length;
    tree->pool = NULL;
    tree->pool_count = 0;
    tree->next = NULL;

    mpack_log("===========================\n");
    mpack_log("initializing tree with data of size %i\n", (int)length);
}
#endif

void mpack_tree_init_pool(mpack_tree_t* tree, const char* data, size_t length,
        mpack_node_data_t* node_pool, size_t node_pool_count)
{
    mpack_tree_init_clear(tree);
    #ifdef MPACK_MALLOC
    tree->next = NULL;
    #endif

    if (node_pool_count == 0) {
        mpack_break("initial page has no nodes!");
        mpack_tree_flag_error(tree, mpack_error_bug);
        return;
    }

    tree->data = data;
    tree->length = length;
    tree->pool = node_pool;
    tree->pool_count = node_pool_count;

    mpack_log("===========================\n");
    mpack_log("initializing tree with data of size %i and pool of count %i\n", (int)length, (int)node_pool_count);
}

void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_tree_init_clear(tree);
    tree->error = error;

    mpack_log("===========================\n");
    mpack_log("initializing tree error state %i\n", (int)error);
}

#if MPACK_STDIO
typedef struct mpack_file_tree_t {
    char* data;
    size_t size;
    char buffer[MPACK_BUFFER_SIZE];
} mpack_file_tree_t;

static void mpack_file_tree_teardown(mpack_tree_t* tree) {
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*)tree->context;
    MPACK_FREE(file_tree->data);
    MPACK_FREE(file_tree);
}

static bool mpack_file_tree_read(mpack_tree_t* tree, mpack_file_tree_t* file_tree, const char* filename, size_t max_size) {

    // open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }

    // get the file size
    errno = 0;
    int error = 0;
    fseek(file, 0, SEEK_END);
    error |= errno;
    long size = ftell(file);
    error |= errno;
    fseek(file, 0, SEEK_SET);
    error |= errno;

    // check for errors
    if (error != 0 || size < 0) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }
    if (size == 0) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_invalid);
        return false;
    }

    // make sure the size is less than max_size
    // (this mess exists to safely convert between long and size_t regardless of their widths)
    if (max_size != 0 && (((uint64_t)LONG_MAX > (uint64_t)SIZE_MAX && size > (long)SIZE_MAX) || (size_t)size > max_size)) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_too_big);
        return false;
    }

    // allocate data
    file_tree->data = (char*)MPACK_MALLOC((size_t)size);
    if (file_tree->data == NULL) {
        fclose(file);
        mpack_tree_init_error(tree, mpack_error_memory);
        return false;
    }

    // read the file
    long total = 0;
    while (total < size) {
        size_t read = fread(file_tree->data + total, 1, (size_t)(size - total), file);
        if (read <= 0) {
            fclose(file);
            mpack_tree_init_error(tree, mpack_error_io);
            MPACK_FREE(file_tree->data);
            return false;
        }
        total += (long)read;
    }

    fclose(file);
    file_tree->size = (size_t)size;
    return true;
}

void mpack_tree_init_file(mpack_tree_t* tree, const char* filename, size_t max_size) {

    // the C STDIO family of file functions use long (e.g. ftell)
    if (max_size > LONG_MAX) {
        mpack_break("max_size of %" PRIu64 " is invalid, maximum is LONG_MAX", (uint64_t)max_size);
        mpack_tree_init_error(tree, mpack_error_bug);
        return;
    }

    // allocate file tree
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*) MPACK_MALLOC(sizeof(mpack_file_tree_t));
    if (file_tree == NULL) {
        mpack_tree_init_error(tree, mpack_error_memory);
        return;
    }

    // read all data
    if (!mpack_file_tree_read(tree, file_tree, filename, max_size)) {
        MPACK_FREE(file_tree);
        return;
    }

    mpack_tree_init(tree, file_tree->data, file_tree->size);
    mpack_tree_set_context(tree, file_tree);
    mpack_tree_set_teardown(tree, mpack_file_tree_teardown);
}
#endif

mpack_error_t mpack_tree_destroy(mpack_tree_t* tree) {
    mpack_tree_cleanup(tree);

    if (tree->teardown)
        tree->teardown(tree);
    tree->teardown = NULL;

    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    if (tree->error == mpack_ok) {
        mpack_log("tree %p setting error %i: %s\n", tree, (int)error, mpack_error_to_string(error));
        tree->error = error;
        if (tree->error_fn)
            tree->error_fn(tree, error);
    }

}



/*
 * Node misc functions
 */

void mpack_node_flag_error(mpack_node_t node, mpack_error_t error) {
    mpack_tree_flag_error(node.tree, error);
}

mpack_tag_t mpack_node_tag(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tag_nil();

    mpack_tag_t tag;
    mpack_memset(&tag, 0, sizeof(tag));

    tag.type = node.data->type;
    switch (node.data->type) {
        case mpack_type_nil:                                            break;
        case mpack_type_bool:    tag.v.b = node.data->value.b;          break;
        case mpack_type_float:   tag.v.f = node.data->value.f;          break;
        case mpack_type_double:  tag.v.d = node.data->value.d;          break;
        case mpack_type_int:     tag.v.i = node.data->value.i;          break;
        case mpack_type_uint:    tag.v.u = node.data->value.u;          break;

        case mpack_type_str:     tag.v.l = node.data->len;     break;
        case mpack_type_bin:     tag.v.l = node.data->len;     break;

        case mpack_type_ext:
            tag.v.l = node.data->len;
            // the exttype of an ext node is stored in the byte preceding the data
            tag.exttype = (int8_t)*(node.data->value.bytes - 1);
            break;

        case mpack_type_array:   tag.v.n = node.data->len;  break;
        case mpack_type_map:     tag.v.n = node.data->len;  break;

        default:
            mpack_assert(0, "unrecognized type %i", (int)node.data->type);
            break;
    }
    return tag;
}

#if MPACK_STDIO
static void mpack_node_print_element(mpack_node_t node, size_t depth, FILE* file) {
    mpack_node_data_t* data = node.data;
    switch (data->type) {

        case mpack_type_nil:
            fprintf(file, "null");
            break;
        case mpack_type_bool:
            fprintf(file, data->value.b ? "true" : "false");
            break;

        case mpack_type_float:
            fprintf(file, "%f", data->value.f);
            break;
        case mpack_type_double:
            fprintf(file, "%f", data->value.d);
            break;

        case mpack_type_int:
            fprintf(file, "%" PRIi64, data->value.i);
            break;
        case mpack_type_uint:
            fprintf(file, "%" PRIu64, data->value.u);
            break;

        case mpack_type_bin:
            fprintf(file, "<binary data of length %u>", data->len);
            break;

        case mpack_type_ext:
            fprintf(file, "<ext data of type %i and length %u>",
                    mpack_node_exttype(node), data->len);
            break;

        case mpack_type_str:
            {
                putc('"', file);
                const char* bytes = mpack_node_data(node);
                for (size_t i = 0; i < data->len; ++i) {
                    char c = bytes[i];
                    switch (c) {
                        case '\n': fprintf(file, "\\n"); break;
                        case '\\': fprintf(file, "\\\\"); break;
                        case '"': fprintf(file, "\\\""); break;
                        default: putc(c, file); break;
                    }
                }
                putc('"', file);
            }
            break;

        case mpack_type_array:
            fprintf(file, "[\n");
            for (size_t i = 0; i < data->len; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_node_print_element(mpack_node_array_at(node, i), depth + 1, file);
                if (i != data->len - 1)
                    putc(',', file);
                putc('\n', file);
            }
            for (size_t i = 0; i < depth; ++i)
                fprintf(file, "    ");
            putc(']', file);
            break;

        case mpack_type_map:
            fprintf(file, "{\n");
            for (size_t i = 0; i < data->len; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_node_print_element(mpack_node_map_key_at(node, i), depth + 1, file);
                fprintf(file, ": ");
                mpack_node_print_element(mpack_node_map_value_at(node, i), depth + 1, file);
                if (i != data->len - 1)
                    putc(',', file);
                putc('\n', file);
            }
            for (size_t i = 0; i < depth; ++i)
                fprintf(file, "    ");
            putc('}', file);
            break;
    }
}

void mpack_node_print_file(mpack_node_t node, FILE* file) {
    mpack_assert(file != NULL, "file is NULL");
    size_t depth = 2;
    for (size_t i = 0; i < depth; ++i)
        fprintf(file, "    ");
    mpack_node_print_element(node, depth, file);
    putc('\n', file);
}
#endif



/*
 * Node Data Functions
 */

void mpack_node_check_utf8(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    mpack_node_data_t* data = node.data;
    if (data->type != mpack_type_str || !mpack_utf8_check(data->value.bytes, data->len))
        mpack_node_flag_error(node, mpack_error_type);
}

void mpack_node_check_utf8_cstr(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    mpack_node_data_t* data = node.data;
    if (data->type != mpack_type_str || !mpack_utf8_check_no_null(data->value.bytes, data->len))
        mpack_node_flag_error(node, mpack_error_type);
}

size_t mpack_node_copy_data(mpack_node_t node, char* buffer, size_t bufsize) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_assert(bufsize == 0 || buffer != NULL, "buffer is NULL for maximum of %i bytes", (int)bufsize);

    mpack_type_t type = node.data->type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    if (node.data->len > bufsize) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    mpack_memcpy(buffer, node.data->value.bytes, node.data->len);
    return (size_t)node.data->len;
}

size_t mpack_node_copy_utf8(mpack_node_t node, char* buffer, size_t bufsize) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_assert(bufsize == 0 || buffer != NULL, "buffer is NULL for maximum of %i bytes", (int)bufsize);

    mpack_type_t type = node.data->type;
    if (type != mpack_type_str) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    if (node.data->len > bufsize) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    if (!mpack_utf8_check(node.data->value.bytes, node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    mpack_memcpy(buffer, node.data->value.bytes, node.data->len);
    return (size_t)node.data->len;
}

void mpack_node_copy_cstr(mpack_node_t node, char* buffer, size_t bufsize) {

    // we can't break here because the error isn't recoverable; we
    // have to add a null-terminator.
    mpack_assert(buffer != NULL, "buffer is NULL");
    mpack_assert(bufsize >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    if (mpack_node_error(node) != mpack_ok) {
        buffer[0] = '\0';
        return;
    }

    if (node.data->type != mpack_type_str) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    if (node.data->len > bufsize - 1) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_too_big);
        return;
    }

    if (!mpack_str_check_no_null(node.data->value.bytes, node.data->len)) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    mpack_memcpy(buffer, node.data->value.bytes, node.data->len);
    buffer[node.data->len] = '\0';
}

void mpack_node_copy_utf8_cstr(mpack_node_t node, char* buffer, size_t bufsize) {

    // we can't break here because the error isn't recoverable; we
    // have to add a null-terminator.
    mpack_assert(buffer != NULL, "buffer is NULL");
    mpack_assert(bufsize >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    if (mpack_node_error(node) != mpack_ok) {
        buffer[0] = '\0';
        return;
    }

    if (node.data->type != mpack_type_str) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    if (node.data->len > bufsize - 1) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_too_big);
        return;
    }

    if (!mpack_utf8_check_no_null(node.data->value.bytes, node.data->len)) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    mpack_memcpy(buffer, node.data->value.bytes, node.data->len);
    buffer[node.data->len] = '\0';
}

#ifdef MPACK_MALLOC
char* mpack_node_data_alloc(mpack_node_t node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure this is a valid data type
    mpack_type_t type = node.data->type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node.data->len > maxlen) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)node.data->len);
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node.data->value.bytes, node.data->len);
    return ret;
}

char* mpack_node_cstr_alloc(mpack_node_t node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure maxlen makes sense
    if (maxlen < 1) {
        mpack_break("maxlen is zero; you must have room for at least a null-terminator");
        mpack_node_flag_error(node, mpack_error_bug);
        return NULL;
    }

    if (node.data->type != mpack_type_str) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node.data->len > maxlen - 1) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    if (!mpack_str_check_no_null(node.data->value.bytes, node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node.data->len + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node.data->value.bytes, node.data->len);
    ret[node.data->len] = '\0';
    return ret;
}

char* mpack_node_utf8_cstr_alloc(mpack_node_t node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure maxlen makes sense
    if (maxlen < 1) {
        mpack_break("maxlen is zero; you must have room for at least a null-terminator");
        mpack_node_flag_error(node, mpack_error_bug);
        return NULL;
    }

    if (node.data->type != mpack_type_str) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    if (node.data->len > maxlen - 1) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    if (!mpack_utf8_check_no_null(node.data->value.bytes, node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node.data->len + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node.data->value.bytes, node.data->len);
    ret[node.data->len] = '\0';
    return ret;
}
#endif


/*
 * Compound Node Functions
 */

static mpack_node_data_t* mpack_node_map_int_impl(mpack_node_t node, int64_t num) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    mpack_node_data_t* found = NULL;

    for (size_t i = 0; i < node.data->len; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);

        if ((key->type == mpack_type_int && key->value.i == num) ||
            (key->type == mpack_type_uint && num >= 0 && key->value.u == (uint64_t)num))
        {
            if (found) {
                mpack_node_flag_error(node, mpack_error_data);
                return NULL;
            }
            found = mpack_node_child(node, i * 2 + 1);
        }
    }

    if (found)
        return found;

    return NULL;
}

static mpack_node_data_t* mpack_node_map_uint_impl(mpack_node_t node, uint64_t num) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    mpack_node_data_t* found = NULL;

    for (size_t i = 0; i < node.data->len; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);

        if ((key->type == mpack_type_uint && key->value.u == num) ||
            (key->type == mpack_type_int && key->value.i >= 0 && (uint64_t)key->value.i == num))
        {
            if (found) {
                mpack_node_flag_error(node, mpack_error_data);
                return NULL;
            }
            found = mpack_node_child(node, i * 2 + 1);
        }
    }

    if (found)
        return found;

    return NULL;
}

static mpack_node_data_t* mpack_node_map_str_impl(mpack_node_t node, const char* str, size_t length) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    mpack_assert(length == 0 || str != NULL, "str of length %i is NULL", (int)length);

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    mpack_node_data_t* found = NULL;

    for (size_t i = 0; i < node.data->len; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);

        if (key->type == mpack_type_str && key->len == length && mpack_memcmp(str, key->value.bytes, length) == 0) {
            if (found) {
                mpack_node_flag_error(node, mpack_error_data);
                return NULL;
            }
            found = mpack_node_child(node, i * 2 + 1);
        }
    }

    if (found)
        return found;

    return NULL;
}

static mpack_node_t mpack_node_wrap_lookup(mpack_tree_t* tree, mpack_node_data_t* data) {
    if (!data) {
        if (tree->error == mpack_ok)
            mpack_tree_flag_error(tree, mpack_error_data);
        return mpack_tree_nil_node(tree);
    }
    return mpack_node(tree, data);
}

static mpack_node_t mpack_node_wrap_lookup_optional(mpack_tree_t* tree, mpack_node_data_t* data) {
    if (!data)
        return mpack_tree_nil_node(tree);
    return mpack_node(tree, data);
}

mpack_node_t mpack_node_map_int(mpack_node_t node, int64_t num) {
    return mpack_node_wrap_lookup(node.tree, mpack_node_map_int_impl(node, num));
}

mpack_node_t mpack_node_map_int_optional(mpack_node_t node, int64_t num) {
    return mpack_node_wrap_lookup_optional(node.tree, mpack_node_map_int_impl(node, num));
}

mpack_node_t mpack_node_map_uint(mpack_node_t node, uint64_t num) {
    return mpack_node_wrap_lookup(node.tree, mpack_node_map_uint_impl(node, num));
}

mpack_node_t mpack_node_map_uint_optional(mpack_node_t node, uint64_t num) {
    return mpack_node_wrap_lookup_optional(node.tree, mpack_node_map_uint_impl(node, num));
}

mpack_node_t mpack_node_map_str(mpack_node_t node, const char* str, size_t length) {
    return mpack_node_wrap_lookup(node.tree, mpack_node_map_str_impl(node, str, length));
}

mpack_node_t mpack_node_map_str_optional(mpack_node_t node, const char* str, size_t length) {
    return mpack_node_wrap_lookup_optional(node.tree, mpack_node_map_str_impl(node, str, length));
}

mpack_node_t mpack_node_map_cstr(mpack_node_t node, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr is NULL");
    return mpack_node_map_str(node, cstr, mpack_strlen(cstr));
}

mpack_node_t mpack_node_map_cstr_optional(mpack_node_t node, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr is NULL");
    return mpack_node_map_str_optional(node, cstr, mpack_strlen(cstr));
}

bool mpack_node_map_contains_int(mpack_node_t node, int64_t num) {
    return mpack_node_map_int_impl(node, num) != NULL;
}

bool mpack_node_map_contains_uint(mpack_node_t node, uint64_t num) {
    return mpack_node_map_uint_impl(node, num) != NULL;
}

bool mpack_node_map_contains_str(mpack_node_t node, const char* str, size_t length) {
    return mpack_node_map_str_impl(node, str, length) != NULL;
}

bool mpack_node_map_contains_cstr(mpack_node_t node, const char* cstr) {
    mpack_assert(cstr != NULL, "cstr is NULL");
    return mpack_node_map_contains_str(node, cstr, mpack_strlen(cstr));
}

size_t mpack_node_enum_optional(mpack_node_t node, const char* strings[], size_t count) {
    if (mpack_node_error(node) != mpack_ok)
        return count;

    // the value is only recognized if it is a string
    if (mpack_node_type(node) != mpack_type_str)
        return count;

    // fetch the string
    const char* key = mpack_node_str(node);
    size_t keylen = mpack_node_strlen(node);
    mpack_assert(mpack_node_error(node) == mpack_ok, "these should not fail");

    // find what key it matches
    for (size_t i = 0; i < count; ++i) {
        const char* other = strings[i];
        size_t otherlen = mpack_strlen(other);
        if (keylen == otherlen && mpack_memcmp(key, other, keylen) == 0)
            return i;
    }

    // no matches
    return count;
}

size_t mpack_node_enum(mpack_node_t node, const char* strings[], size_t count) {
    size_t value = mpack_node_enum_optional(node, strings, count);
    if (value == count)
        mpack_node_flag_error(node, mpack_error_type);
    return value;
}

#endif


