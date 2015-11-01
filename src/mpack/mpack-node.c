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

#define MPACK_INTERNAL 1

#include "mpack-node.h"

#if MPACK_NODE



/*
 * Tree Parsing
 */

typedef struct mpack_level_t {
    mpack_node_data_t* child;
    size_t left; // children left in level
} mpack_level_t;

typedef struct mpack_tree_parser_t {
    mpack_tree_t* tree;
    const char* data;
    size_t left; // bytes left in data
    size_t possible_nodes_left;

    size_t level;
    size_t depth;
    mpack_level_t* stack;
    bool stack_allocated;
} mpack_tree_parser_t;

MPACK_STATIC_INLINE_SPEED uint8_t mpack_tree_u8(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint8_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint8_t val = mpack_load_native_u8(parser->data);
    parser->data += sizeof(uint8_t);
    parser->left -= sizeof(uint8_t);
    parser->possible_nodes_left -= sizeof(uint8_t);
    return val;
}

MPACK_STATIC_INLINE_SPEED uint16_t mpack_tree_u16(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint16_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint16_t val = mpack_load_native_u16(parser->data);
    parser->data += sizeof(uint16_t);
    parser->left -= sizeof(uint16_t);
    parser->possible_nodes_left -= sizeof(uint16_t);
    return val;
}

MPACK_STATIC_INLINE_SPEED uint32_t mpack_tree_u32(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint32_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint32_t val = mpack_load_native_u32(parser->data);
    parser->data += sizeof(uint32_t);
    parser->left -= sizeof(uint32_t);
    parser->possible_nodes_left -= sizeof(uint32_t);
    return val;
}

MPACK_STATIC_INLINE_SPEED uint64_t mpack_tree_u64(mpack_tree_parser_t* parser) {
    if (parser->possible_nodes_left < sizeof(uint64_t)) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        return 0;
    }
    uint64_t val = mpack_load_native_u64(parser->data);
    parser->data += sizeof(uint64_t);
    parser->left -= sizeof(uint64_t);
    parser->possible_nodes_left -= sizeof(uint64_t);
    return val;
}

MPACK_STATIC_INLINE int8_t  mpack_tree_i8 (mpack_tree_parser_t* parser) {return (int8_t) mpack_tree_u8(parser); }
MPACK_STATIC_INLINE int16_t mpack_tree_i16(mpack_tree_parser_t* parser) {return (int16_t)mpack_tree_u16(parser);}
MPACK_STATIC_INLINE int32_t mpack_tree_i32(mpack_tree_parser_t* parser) {return (int32_t)mpack_tree_u32(parser);}
MPACK_STATIC_INLINE int64_t mpack_tree_i64(mpack_tree_parser_t* parser) {return (int64_t)mpack_tree_u64(parser);}

MPACK_STATIC_INLINE_SPEED float mpack_tree_float(mpack_tree_parser_t* parser) {
    union {
        float f;
        uint32_t i;
    } u;
    u.i = mpack_tree_u32(parser);
    return u.f;
}

MPACK_STATIC_INLINE_SPEED double mpack_tree_double(mpack_tree_parser_t* parser) {
    union {
        double d;
        uint64_t i;
    } u;
    u.i = mpack_tree_u64(parser);
    return u.d;
}

static void mpack_tree_parse_children(mpack_tree_parser_t* parser, mpack_node_data_t* node) {
    mpack_type_t type = node->type;
    size_t total = node->value.content.n;

    // Make sure we have enough room in the stack
    if (parser->level + 1 == parser->depth) {
        #ifdef MPACK_MALLOC
        size_t new_depth = parser->depth * 2;
        mpack_log("growing stack to depth %i\n", (int)new_depth);

        // Replace the stack-allocated parsing stack
        if (parser->stack_allocated) {
            mpack_level_t* new_stack = (mpack_level_t*)MPACK_MALLOC(sizeof(mpack_level_t) * new_depth);
            if (!new_stack) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                parser->level = 0;
                return;
            }
            memcpy(new_stack, parser->stack, sizeof(mpack_level_t) * parser->depth);
            parser->stack = new_stack;
            parser->stack_allocated = false;

        // Realloc the allocated parsing stack
        } else {
            parser->stack = (mpack_level_t*)mpack_realloc(parser->stack, sizeof(mpack_level_t) * parser->depth, sizeof(mpack_level_t) * new_depth);
            if (!parser->stack) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                parser->level = 0;
                return;
            }
        }
        parser->depth = new_depth;
        #else
        mpack_tree_flag_error(parser->tree, mpack_error_too_big);
        parser->level = 0;
        return;
        #endif
    }

    // Calculate total elements to read
    if (type == mpack_type_map) {
        if ((uint64_t)total * 2 > (uint64_t)SIZE_MAX) {
            mpack_tree_flag_error(parser->tree, mpack_error_too_big);
            parser->level = 0;
            return;
        }
        total *= 2;
    }

    // Each node is at least one byte. Count these bytes now to make
    // sure there is enough data left.
    if (total > parser->possible_nodes_left) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        parser->level = 0;
        return;
    }
    parser->possible_nodes_left -= total;

    // If there are enough nodes left in the current page, no need to grow
    if (total <= parser->tree->page.left) {
        node->value.content.children = parser->tree->page.nodes + parser->tree->page.pos;
        parser->tree->page.pos += total;
        parser->tree->page.left -= total;

    } else {

        #ifdef MPACK_MALLOC

        // We can't grow if we're using a fixed pool
        if (!parser->tree->owned) {
            mpack_tree_flag_error(parser->tree, mpack_error_too_big);
            parser->level = 0;
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

        // Allocate the new link first. The two cases below put it into the list before trying
        // to allocate its nodes so it gets freed later in case of allocation failure.
        mpack_tree_link_t* link = (mpack_tree_link_t*)MPACK_MALLOC(sizeof(mpack_tree_link_t));
        if (link == NULL) {
            mpack_tree_flag_error(parser->tree, mpack_error_memory);
            parser->level = 0;
            return;
        }

        if (total > MPACK_NODE_PAGE_SIZE || parser->tree->page.left > MPACK_NODE_PAGE_SIZE / 8) {
            mpack_log("allocating seperate page for %i children, %i left in page of size %i\n",
                    (int)total, (int)parser->tree->page.left, (int)MPACK_NODE_PAGE_SIZE);

            // Allocate only this node's children and insert it after the current page
            link->next = parser->tree->page.next;
            parser->tree->page.next = link;
            link->nodes = (mpack_node_data_t*)MPACK_MALLOC(sizeof(mpack_node_data_t) * total);
            if (link->nodes == NULL) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                parser->level = 0;
                return;
            }

            // Use the new page for the node's children. pos and left are not used.
            node->value.content.children = link->nodes;

        } else {
            mpack_log("allocating new page for %i children, wasting %i in page of size %i\n",
                    (int)total, (int)parser->tree->page.left, (int)MPACK_NODE_PAGE_SIZE);

            // Move the current page into the new link, and allocate a new page
            *link = parser->tree->page;
            parser->tree->page.next = link;
            parser->tree->page.nodes = (mpack_node_data_t*)MPACK_MALLOC(sizeof(mpack_node_data_t) * MPACK_NODE_PAGE_SIZE);
            if (parser->tree->page.nodes == NULL) {
                mpack_tree_flag_error(parser->tree, mpack_error_memory);
                parser->level = 0;
                return;
            }

            // Take this node's children from the page
            node->value.content.children = parser->tree->page.nodes;
            parser->tree->page.pos = total;
            parser->tree->page.left = MPACK_NODE_PAGE_SIZE - total;
        }

        #else
        // We can't grow if we don't have an allocator
        mpack_tree_flag_error(parser->tree, mpack_error_too_big);
        parser->level = 0;
        return;
        #endif
    }

    // Push this node onto the stack to read its children
    ++parser->level;
    parser->stack[parser->level].child = node->value.content.children;
    parser->stack[parser->level].left = total;
}

static void mpack_tree_parse_bytes(mpack_tree_parser_t* parser, mpack_node_data_t* node) {
    size_t length = node->value.data.l;
    if (length > parser->possible_nodes_left) {
        mpack_tree_flag_error(parser->tree, mpack_error_invalid);
        parser->level = 0;
        return;
    }
    node->value.data.bytes = parser->data;
    parser->data += length;
    parser->left -= length;
    parser->possible_nodes_left -= length;
}

static void mpack_tree_parse(mpack_tree_t* tree, const char* data, size_t length) {
    mpack_log("starting parse\n");

    // This function is unfortunately huge and ugly, but there isn't
    // a good way to break it apart without losing performance. It's
    // well-commented to try to make up for it.

    if (length == 0) {
        mpack_tree_init_error(tree, mpack_error_invalid);
        return;
    }
    if (tree->page.left == 0) {
        mpack_break("initial page has no nodes!");
        mpack_tree_init_error(tree, mpack_error_bug);
        return;
    }
    tree->root = tree->page.nodes + tree->page.pos;
    ++tree->page.pos;
    --tree->page.left;

    // Setup parser
    mpack_tree_parser_t parser;
    mpack_memset(&parser, 0, sizeof(parser));
    parser.tree = tree;
    parser.data = data;
    parser.left = length;

    // We read nodes in a loop instead of recursively for maximum
    // performance. The stack holds the amount of children left to
    // read in each level of the tree.

    // Even when we have a malloc() function, it's much faster to
    // allocate the initial parsing stack on the call stack. We
    // replace it with a heap allocation if we need to grow it.
    #ifdef MPACK_MALLOC
    static const size_t initial_depth = MPACK_NODE_INITIAL_DEPTH;
    parser.stack_allocated = true;
    #else
    static const size_t initial_depth = MPACK_NODE_MAX_DEPTH_WITHOUT_MALLOC;
    #endif

    mpack_level_t stack_[initial_depth];
    parser.depth = initial_depth;
    parser.stack = stack_;

    // We keep track of the number of possible nodes left in the data. This
    // is to ensure that malicious nested data is not trying to make us
    // run out of memory by allocating too many nodes. (For example malicious
    // data that repeats 0xDE 0xFF 0xFF would otherwise cause us to run out
    // of memory. With this, the parser can only allocate as many nodes as
    // there are bytes in the data (plus the paging overhead, 12%.) An error
    // will be flagged immediately if and when there isn't enough data left
    // to fully read all children of all open compound types on the stack.)
    parser.possible_nodes_left = length;

    // configure the root node
    --parser.possible_nodes_left;
    tree->node_count = 1;
    parser.level = 0;
    parser.stack[0].child = tree->root;
    parser.stack[0].left = 1;

    do {
        mpack_node_data_t* node = parser.stack[parser.level].child;
        --parser.stack[parser.level].left;
        ++parser.stack[parser.level].child;

        // read the type (we've already counted this byte in possible_nodes_left)
        ++parser.possible_nodes_left;
        uint8_t type = mpack_tree_u8(&parser);

        // as with mpack_read_tag(), the fastest way to parse a node is to switch
        // on the first byte, and to explicitly list every possible byte.
        switch (type) {

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
                break;

            // negative fixnum
            case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
            case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
            case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
            case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
                node->type = mpack_type_int;
                node->value.i = (int8_t)type;
                break;

            // fixmap
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
            case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
                node->type = mpack_type_map;
                node->value.content.n = type & ~0xf0;
                mpack_tree_parse_children(&parser, node);
                break;

            // fixarray
            case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
            case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
                node->type = mpack_type_array;
                node->value.content.n = type & ~0xf0;
                mpack_tree_parse_children(&parser, node);
                break;

            // fixstr
            case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
            case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
            case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
            case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
                node->type = mpack_type_str;
                node->value.data.l = type & ~0xe0;
                mpack_tree_parse_bytes(&parser, node);
                break;

            // nil
            case 0xc0:
                node->type = mpack_type_nil;
                break;

            // bool
            case 0xc2: case 0xc3:
                node->type = mpack_type_bool;
                node->value.b = type & 1;
                break;

            // bin8
            case 0xc4:
                node->type = mpack_type_bin;
                node->value.data.l = mpack_tree_u8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // bin16
            case 0xc5:
                node->type = mpack_type_bin;
                node->value.data.l = mpack_tree_u16(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // bin32
            case 0xc6:
                node->type = mpack_type_bin;
                node->value.data.l = mpack_tree_u32(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // ext8
            case 0xc7:
                node->type = mpack_type_ext;
                node->value.data.l = mpack_tree_u8(&parser);
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // ext16
            case 0xc8:
                node->type = mpack_type_ext;
                node->value.data.l = mpack_tree_u16(&parser);
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // ext32
            case 0xc9:
                node->type = mpack_type_ext;
                node->value.data.l = mpack_tree_u32(&parser);
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // float
            case 0xca:
                node->type = mpack_type_float;
                node->value.f = mpack_tree_float(&parser);
                break;

            // double
            case 0xcb:
                node->type = mpack_type_double;
                node->value.d = mpack_tree_double(&parser);
                break;

            // uint8
            case 0xcc:
                node->type = mpack_type_uint;
                node->value.u = mpack_tree_u8(&parser);
                break;

            // uint16
            case 0xcd:
                node->type = mpack_type_uint;
                node->value.u = mpack_tree_u16(&parser);
                break;

            // uint32
            case 0xce:
                node->type = mpack_type_uint;
                node->value.u = mpack_tree_u32(&parser);
                break;

            // uint64
            case 0xcf:
                node->type = mpack_type_uint;
                node->value.u = mpack_tree_u64(&parser);
                break;

            // int8
            case 0xd0:
                node->type = mpack_type_int;
                node->value.i = mpack_tree_i8(&parser);
                break;

            // int16
            case 0xd1:
                node->type = mpack_type_int;
                node->value.i = mpack_tree_i16(&parser);
                break;

            // int32
            case 0xd2:
                node->type = mpack_type_int;
                node->value.i = mpack_tree_i32(&parser);
                break;

            // int64
            case 0xd3:
                node->type = mpack_type_int;
                node->value.i = mpack_tree_i64(&parser);
                break;

            // fixext1
            case 0xd4:
                node->type = mpack_type_ext;
                node->value.data.l = 1;
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // fixext2
            case 0xd5:
                node->type = mpack_type_ext;
                node->value.data.l = 2;
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // fixext4
            case 0xd6:
                node->type = mpack_type_ext;
                node->value.data.l = 4;
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // fixext8
            case 0xd7:
                node->type = mpack_type_ext;
                node->value.data.l = 8;
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // fixext16
            case 0xd8:
                node->type = mpack_type_ext;
                node->value.data.l = 16;
                node->exttype = mpack_tree_i8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // str8
            case 0xd9:
                node->type = mpack_type_str;
                node->value.data.l = mpack_tree_u8(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // str16
            case 0xda:
                node->type = mpack_type_str;
                node->value.data.l = mpack_tree_u16(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // str32
            case 0xdb:
                node->type = mpack_type_str;
                node->value.data.l = mpack_tree_u32(&parser);
                mpack_tree_parse_bytes(&parser, node);
                break;

            // array16
            case 0xdc:
                node->type = mpack_type_array;
                node->value.content.n = mpack_tree_u16(&parser);
                mpack_tree_parse_children(&parser, node);
                break;

            // array32
            case 0xdd:
                node->type = mpack_type_array;
                node->value.content.n = mpack_tree_u32(&parser);
                mpack_tree_parse_children(&parser, node);
                break;

            // map16
            case 0xde:
                node->type = mpack_type_map;
                node->value.content.n = mpack_tree_u16(&parser);
                mpack_tree_parse_children(&parser, node);
                break;

            // map32
            case 0xdf:
                node->type = mpack_type_map;
                node->value.content.n = mpack_tree_u32(&parser);
                mpack_tree_parse_children(&parser, node);
                break;

            // reserved
            case 0xc1:
                mpack_tree_flag_error(tree, mpack_error_invalid);
                break;
        }

        // Pop any empty compound types from the stack
        while (parser.level != 0 && parser.stack[parser.level].left == 0)
            --parser.level;
    } while (parser.level != 0 && mpack_tree_error(parser.tree) == mpack_ok);

    #ifdef MPACK_MALLOC
    if (!parser.stack_allocated)
        MPACK_FREE(parser.stack);
    #endif

    tree->size = length - parser.left;
    mpack_log("parsed tree of %i bytes, %i bytes left\n", (int)tree->size, (int)parser.left);
    mpack_log("%i nodes in final page\n", (int)tree->page.pos);

    // This seems like a bug / performance flaw in GCC. In release the
    // below assert would compile to:
    //
    //     (!(mpack_tree_error(parser.tree) != mpack_ok || possible_nodes_left == remaining) ? __builtin_unreachable() : ((void)0))
    //
    // This produces identical assembly with GCC 5.1 on ARM64 under -O3, but
    // with -O3 -flto, node parsing is over 4% slower. This should be a no-op
    // even in -flto since the function ends here and possible_nodes_left
    // does not escape this function.
    //
    // Leaving a TODO: here to explore this further. In the meantime we preproc it
    // under MPACK_DEBUG.
    #if MPACK_DEBUG
    mpack_assert(mpack_tree_error(parser.tree) != mpack_ok || parser.possible_nodes_left == parser.left,
            "incorrect calculation of possible nodes! %i possible nodes, but %i bytes remaining",
            (int)parser.possible_nodes_left, (int)parser.left);
    #endif
}



/*
 * Tree functions
 */

mpack_node_t mpack_tree_root(mpack_tree_t* tree) {
    return mpack_node(tree, (mpack_tree_error(tree) != mpack_ok) ? &tree->nil_node : tree->root);
}

static void mpack_tree_init_clear(mpack_tree_t* tree) {
    mpack_memset(tree, 0, sizeof(*tree));
    tree->nil_node.type = mpack_type_nil;
}

#ifdef MPACK_MALLOC
void mpack_tree_init(mpack_tree_t* tree, const char* data, size_t length) {
    mpack_tree_init_clear(tree);
    tree->owned = true;

    // allocate first page
    mpack_log("allocating initial page of size %i\n", (int)MPACK_NODE_PAGE_SIZE);
    tree->page.nodes = (mpack_node_data_t*)MPACK_MALLOC(sizeof(mpack_node_data_t) * MPACK_NODE_PAGE_SIZE);
    if (tree->page.nodes == NULL) {
        tree->error = mpack_error_memory;
        return;
    }
    tree->page.next = NULL;
    tree->page.pos = 0;
    tree->page.left = MPACK_NODE_PAGE_SIZE;

    mpack_tree_parse(tree, data, length);
}
#endif

void mpack_tree_init_pool(mpack_tree_t* tree, const char* data, size_t length, mpack_node_data_t* node_pool, size_t node_pool_count) {
    mpack_tree_init_clear(tree);

    tree->page.next = NULL;
    tree->page.nodes = node_pool;
    tree->page.pos = 0;
    tree->page.left = node_pool_count;

    mpack_tree_parse(tree, data, length);
}

void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_tree_init_clear(tree);
    tree->error = error;
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
    file_tree->data = (char*)MPACK_MALLOC(size);
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
        total += read;
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
    #ifdef MPACK_MALLOC
    if (tree->owned) {
        if (tree->page.nodes)
            MPACK_FREE(tree->page.nodes);
        mpack_tree_link_t* link = tree->page.next;
        while (link) {
            mpack_tree_link_t* next = link->next;
            if (link->nodes)
                MPACK_FREE(link->nodes);
            MPACK_FREE(link);
            link = next;
        }
    }
    #endif

    if (tree->teardown)
        tree->teardown(tree);
    tree->teardown = NULL;

    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_log("tree %p setting error %i: %s\n", tree, (int)error, mpack_error_to_string(error));

    if (tree->error == mpack_ok) {
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

        case mpack_type_str:     tag.v.l = node.data->value.data.l;     break;
        case mpack_type_bin:     tag.v.l = node.data->value.data.l;     break;

        case mpack_type_ext:
            tag.v.l = node.data->value.data.l;
            tag.exttype = node.data->exttype;
            break;

        case mpack_type_array:   tag.v.n = node.data->value.content.n;  break;
        case mpack_type_map:     tag.v.n = node.data->value.content.n;  break;
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
            fprintf(file, "<binary data of length %u>", data->value.data.l);
            break;

        case mpack_type_ext:
            fprintf(file, "<ext data of type %i and length %u>", data->exttype, data->value.data.l);
            break;

        case mpack_type_str:
            {
                putc('"', file);
                const char* bytes = mpack_node_data(node);
                for (size_t i = 0; i < data->value.data.l; ++i) {
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
            for (size_t i = 0; i < data->value.content.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_node_print_element(mpack_node_array_at(node, i), depth + 1, file);
                if (i != data->value.content.n - 1)
                    putc(',', file);
                putc('\n', file);
            }
            for (size_t i = 0; i < depth; ++i)
                fprintf(file, "    ");
            putc(']', file);
            break;

        case mpack_type_map:
            fprintf(file, "{\n");
            for (size_t i = 0; i < data->value.content.n; ++i) {
                for (size_t j = 0; j < depth + 1; ++j)
                    fprintf(file, "    ");
                mpack_node_print_element(mpack_node_map_key_at(node, i), depth + 1, file);
                fprintf(file, ": ");
                mpack_node_print_element(mpack_node_map_value_at(node, i), depth + 1, file);
                if (i != data->value.content.n - 1)
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
    int depth = 2;
    for (int i = 0; i < depth; ++i)
        fprintf(file, "    ");
    mpack_node_print_element(node, depth, file);
    putc('\n', file);
}
#endif



/*
 * Node Data Functions
 */

size_t mpack_node_copy_data(mpack_node_t node, char* buffer, size_t size) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_type_t type = node.data->type;
    if (type != mpack_type_str && type != mpack_type_bin && type != mpack_type_ext) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    if (node.data->value.data.l > size) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    mpack_memcpy(buffer, node.data->value.data.bytes, node.data->value.data.l);
    return (size_t)node.data->value.data.l;
}

void mpack_node_copy_cstr(mpack_node_t node, char* buffer, size_t size) {
    if (mpack_node_error(node) != mpack_ok)
        return;

    // we can't break here because the error isn't recoverable; we
    // have to add a null-terminator.
    mpack_assert(size >= 1, "buffer size is zero; you must have room for at least a null-terminator");

    if (node.data->type != mpack_type_str) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    if (node.data->value.data.l > size - 1) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_too_big);
        return;
    }

    mpack_memcpy(buffer, node.data->value.data.bytes, node.data->value.data.l);
    buffer[node.data->value.data.l] = '\0';
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

    if (node.data->value.data.l > maxlen) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)node.data->value.data.l);
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node.data->value.data.bytes, node.data->value.data.l);
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

    if (node.data->value.data.l > maxlen - 1) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node.data->value.data.l + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, node.data->value.data.bytes, node.data->value.data.l);
    ret[node.data->value.data.l] = '\0';
    return ret;
}
#endif


/*
 * Compound Node Functions
 */

mpack_node_t mpack_node_map_int_impl(mpack_node_t node, int64_t num, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tree_nil_node(node.tree);

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return mpack_tree_nil_node(node.tree);
    }

    for (size_t i = 0; i < node.data->value.content.n; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);
        mpack_node_data_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->type == mpack_type_int && key->value.i == num)
            return mpack_node(node.tree, value);
        if (key->type == mpack_type_uint && num >= 0 && key->value.u == (uint64_t)num)
            return mpack_node(node.tree, value);
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return mpack_tree_nil_node(node.tree);
}

mpack_node_t mpack_node_map_uint_impl(mpack_node_t node, uint64_t num, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tree_nil_node(node.tree);

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return mpack_tree_nil_node(node.tree);
    }

    for (size_t i = 0; i < node.data->value.content.n; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);
        mpack_node_data_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->type == mpack_type_uint && key->value.u == num)
            return mpack_node(node.tree, value);
        if (key->type == mpack_type_int && key->value.i >= 0 && (uint64_t)key->value.i == num)
            return mpack_node(node.tree, value);
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return mpack_tree_nil_node(node.tree);
}

mpack_node_t mpack_node_map_str_impl(mpack_node_t node, const char* str, size_t length, bool optional) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tree_nil_node(node.tree);

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return mpack_tree_nil_node(node.tree);
    }

    for (size_t i = 0; i < node.data->value.content.n; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);
        mpack_node_data_t* value = mpack_node_child(node, i * 2 + 1);

        if (key->type == mpack_type_str && key->value.data.l == length && mpack_memcmp(str, key->value.data.bytes, length) == 0)
            return mpack_node(node.tree, value);
    }

    if (!optional)
        mpack_node_flag_error(node, mpack_error_data);
    return mpack_tree_nil_node(node.tree);
}

bool mpack_node_map_contains_str(mpack_node_t node, const char* str, size_t length) {
    if (mpack_node_error(node) != mpack_ok)
        return false;

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return false;
    }

    for (size_t i = 0; i < node.data->value.content.n; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);
        if (key->type == mpack_type_str && key->value.data.l == length && mpack_memcmp(str, key->value.data.bytes, length) == 0)
            return true;
    }

    return false;
}


#endif


