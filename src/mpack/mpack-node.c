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

#define MPACK_INTERNAL 1

#include "mpack-node.h"

MPACK_SILENCE_WARNINGS_BEGIN

#if MPACK_NODE

MPACK_STATIC_INLINE const char* mpack_node_data_unchecked(mpack_node_t node) {
    mpack_assert(mpack_node_error(node) == mpack_ok, "tree is in an error state!");

    mpack_type_t type = node.data->type;
    MPACK_UNUSED(type);
    #if MPACK_EXTENSIONS
    mpack_assert(type == mpack_type_str || type == mpack_type_bin || type == mpack_type_ext,
            "node of type %i (%s) is not a data type!", type, mpack_type_to_string(type));
    #else
    mpack_assert(type == mpack_type_str || type == mpack_type_bin,
            "node of type %i (%s) is not a data type!", type, mpack_type_to_string(type));
    #endif

    return node.tree->data + node.data->value.offset;
}

#if MPACK_EXTENSIONS
MPACK_STATIC_INLINE int8_t mpack_node_exttype_unchecked(mpack_node_t node) {
    mpack_assert(mpack_node_error(node) == mpack_ok, "tree is in an error state!");

    mpack_type_t type = node.data->type;
    MPACK_UNUSED(type);
    mpack_assert(type == mpack_type_ext, "node of type %i (%s) is not an ext type!",
            type, mpack_type_to_string(type));

    // the exttype of an ext node is stored in the byte preceding the data
    return mpack_load_i8(mpack_node_data_unchecked(node) - 1);
}
#endif



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

#ifdef MPACK_MALLOC
/*
 * Fills the tree until we have at least enough bytes for the current node.
 */
static bool mpack_tree_reserve_fill(mpack_tree_t* tree) {
    mpack_assert(tree->parser.state == mpack_tree_parse_state_in_progress);

    size_t bytes = tree->parser.current_node_reserved;
    mpack_assert(bytes > tree->parser.possible_nodes_left,
            "there are already enough bytes! call mpack_tree_ensure() instead.");
    mpack_log("filling to reserve %i bytes\n", (int)bytes);

    // if the necessary bytes would put us over the maximum tree
    // size, fail right away.
    // TODO: check for overflow?
    if (tree->data_length + bytes > tree->max_size) {
        mpack_tree_flag_error(tree, mpack_error_too_big);
        return false;
    }

    // we'll need a read function to fetch more data. if there's
    // no read function, the data should contain an entire message
    // (or messages), so we flag it as invalid.
    if (tree->read_fn == NULL) {
        mpack_log("tree has no read function!\n");
        mpack_tree_flag_error(tree, mpack_error_invalid);
        return false;
    }

    // expand the buffer if needed
    if (tree->data_length + bytes > tree->buffer_capacity) {

        // TODO: check for overflow?
        size_t new_capacity = (tree->buffer_capacity == 0) ? MPACK_BUFFER_SIZE : tree->buffer_capacity;
        while (new_capacity < tree->data_length + bytes)
            new_capacity *= 2;
        if (new_capacity > tree->max_size)
            new_capacity = tree->max_size;

        mpack_log("expanding buffer from %i to %i\n", (int)tree->buffer_capacity, (int)new_capacity);

        char* new_buffer;
        if (tree->buffer == NULL)
            new_buffer = (char*)MPACK_MALLOC(new_capacity);
        else
            new_buffer = (char*)mpack_realloc(tree->buffer, tree->data_length, new_capacity);

        if (new_buffer == NULL) {
            mpack_tree_flag_error(tree, mpack_error_memory);
            return false;
        }

        tree->data = new_buffer;
        tree->buffer = new_buffer;
        tree->buffer_capacity = new_capacity;
    }

    // request as much data as possible, looping until we have
    // all the data we need
    do {
        size_t read = tree->read_fn(tree, tree->buffer + tree->data_length, tree->buffer_capacity - tree->data_length);

        // If the fill function encounters an error, it should flag an error on
        // the tree.
        if (mpack_tree_error(tree) != mpack_ok)
            return false;

        // We guard against fill functions that return -1 just in case.
        if (read == (size_t)(-1)) {
            mpack_tree_flag_error(tree, mpack_error_io);
            return false;
        }

        // If the fill function returns 0, the data is not available yet. We
        // return false to stop parsing the current node.
        if (read == 0) {
            mpack_log("not enough data.\n");
            return false;
        }

        mpack_log("read %" PRIu32 " more bytes\n", (uint32_t)read);
        tree->data_length += read;
        tree->parser.possible_nodes_left += read;
    } while (tree->parser.possible_nodes_left < bytes);

    return true;
}
#endif

/*
 * Ensures there are enough additional bytes in the tree for the current node
 * (including reserved bytes for the children of this node, and in addition to
 * the reserved bytes for children of previous compound nodes), reading more
 * data if needed.
 *
 * extra_bytes is the number of additional bytes to reserve for the current
 * node beyond the type byte (since one byte is already reserved for each node
 * by its parent array or map.)
 *
 * This may reallocate the tree, which means the tree->data pointer may change!
 *
 * Returns false if not enough bytes could be read.
 */
MPACK_STATIC_INLINE bool mpack_tree_reserve_bytes(mpack_tree_t* tree, size_t extra_bytes) {
    mpack_assert(tree->parser.state == mpack_tree_parse_state_in_progress);

    // We guard against overflow here. A compound type could declare more than
    // MPACK_UINT32_MAX contents which overflows SIZE_MAX on 32-bit platforms. We
    // flag mpack_error_invalid instead of mpack_error_too_big since it's far
    // more likely that the message is corrupt than that the data is valid but
    // not parseable on this architecture (see test_read_node_possible() in
    // test-node.c .)
    if ((uint64_t)tree->parser.current_node_reserved + (uint64_t)extra_bytes > SIZE_MAX) {
        mpack_tree_flag_error(tree, mpack_error_invalid);
        return false;
    }

    tree->parser.current_node_reserved += extra_bytes;

    // Note that possible_nodes_left already accounts for reserved bytes for
    // children of previous compound nodes. So even if there are hundreds of
    // bytes left in the buffer, we might need to read anyway.
    if (tree->parser.current_node_reserved <= tree->parser.possible_nodes_left)
        return true;

    #ifdef MPACK_MALLOC
    return mpack_tree_reserve_fill(tree);
    #else
    return false;
    #endif
}

MPACK_STATIC_INLINE size_t mpack_tree_parser_stack_capacity(mpack_tree_t* tree) {
    #ifdef MPACK_MALLOC
    return tree->parser.stack_capacity;
    #else
    return sizeof(tree->parser.stack) / sizeof(tree->parser.stack[0]);
    #endif
}

static bool mpack_tree_push_stack(mpack_tree_t* tree, mpack_node_data_t* first_child, size_t total) {
    mpack_tree_parser_t* parser = &tree->parser;
    mpack_assert(parser->state == mpack_tree_parse_state_in_progress);

    // No need to push empty containers
    if (total == 0)
        return true;

    // Make sure we have enough room in the stack
    if (parser->level + 1 == mpack_tree_parser_stack_capacity(tree)) {
        #ifdef MPACK_MALLOC
        size_t new_capacity = parser->stack_capacity * 2;
        mpack_log("growing parse stack to capacity %i\n", (int)new_capacity);

        // Replace the stack-allocated parsing stack
        if (!parser->stack_owned) {
            mpack_level_t* new_stack = (mpack_level_t*)MPACK_MALLOC(sizeof(mpack_level_t) * new_capacity);
            if (!new_stack) {
                mpack_tree_flag_error(tree, mpack_error_memory);
                return false;
            }
            mpack_memcpy(new_stack, parser->stack, sizeof(mpack_level_t) * parser->stack_capacity);
            parser->stack = new_stack;
            parser->stack_owned = true;

        // Realloc the allocated parsing stack
        } else {
            mpack_level_t* new_stack = (mpack_level_t*)mpack_realloc(parser->stack,
                    sizeof(mpack_level_t) * parser->stack_capacity, sizeof(mpack_level_t) * new_capacity);
            if (!new_stack) {
                mpack_tree_flag_error(tree, mpack_error_memory);
                return false;
            }
            parser->stack = new_stack;
        }
        parser->stack_capacity = new_capacity;
        #else
        mpack_tree_flag_error(tree, mpack_error_too_big);
        return false;
        #endif
    }

    // Push the contents of this node onto the parsing stack
    ++parser->level;
    parser->stack[parser->level].child = first_child;
    parser->stack[parser->level].left = total;
    return true;
}

static bool mpack_tree_parse_children(mpack_tree_t* tree, mpack_node_data_t* node) {
    mpack_tree_parser_t* parser = &tree->parser;
    mpack_assert(parser->state == mpack_tree_parse_state_in_progress);

    mpack_type_t type = node->type;
    size_t total = node->len;

    // Calculate total elements to read
    if (type == mpack_type_map) {
        if ((uint64_t)total * 2 > SIZE_MAX) {
            mpack_tree_flag_error(tree, mpack_error_too_big);
            return false;
        }
        total *= 2;
    }

    // Make sure we are under our total node limit (TODO can this overflow?)
    tree->node_count += total;
    if (tree->node_count > tree->max_nodes) {
        mpack_tree_flag_error(tree, mpack_error_too_big);
        return false;
    }

    // Each node is at least one byte. Count these bytes now to make
    // sure there is enough data left.
    if (!mpack_tree_reserve_bytes(tree, total))
        return false;

    // If there are enough nodes left in the current page, no need to grow
    if (total <= parser->nodes_left) {
        node->value.children = parser->nodes;
        parser->nodes += total;
        parser->nodes_left -= total;

    } else {

        #ifdef MPACK_MALLOC

        // We can't grow if we're using a fixed pool (i.e. we didn't start with a page)
        if (!tree->next) {
            mpack_tree_flag_error(tree, mpack_error_too_big);
            return false;
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
            // TODO: this should check for overflow
            page = (mpack_tree_page_t*)MPACK_MALLOC(
                    sizeof(mpack_tree_page_t) + sizeof(mpack_node_data_t) * (total - 1));
            if (page == NULL) {
                mpack_tree_flag_error(tree, mpack_error_memory);
                return false;
            }
            mpack_log("allocated seperate page %p for %i children, %i left in page of %i total\n",
                    (void*)page, (int)total, (int)parser->nodes_left, (int)MPACK_NODES_PER_PAGE);

            node->value.children = page->nodes;

        } else {
            page = (mpack_tree_page_t*)MPACK_MALLOC(MPACK_PAGE_ALLOC_SIZE);
            if (page == NULL) {
                mpack_tree_flag_error(tree, mpack_error_memory);
                return false;
            }
            mpack_log("allocated new page %p for %i children, wasting %i in page of %i total\n",
                    (void*)page, (int)total, (int)parser->nodes_left, (int)MPACK_NODES_PER_PAGE);

            node->value.children = page->nodes;
            parser->nodes = page->nodes + total;
            parser->nodes_left = MPACK_NODES_PER_PAGE - total;
        }

        page->next = tree->next;
        tree->next = page;

        #else
        // We can't grow if we don't have an allocator
        mpack_tree_flag_error(tree, mpack_error_too_big);
        return false;
        #endif
    }

    return mpack_tree_push_stack(tree, node->value.children, total);
}

static bool mpack_tree_parse_bytes(mpack_tree_t* tree, mpack_node_data_t* node) {
    node->value.offset = tree->size + tree->parser.current_node_reserved + 1;
    return mpack_tree_reserve_bytes(tree, node->len);
}

#if MPACK_EXTENSIONS
static bool mpack_tree_parse_ext(mpack_tree_t* tree, mpack_node_data_t* node) {
    // reserve space for exttype
    tree->parser.current_node_reserved += sizeof(int8_t);
    node->type = mpack_type_ext;
    return mpack_tree_parse_bytes(tree, node);
}
#endif

static bool mpack_tree_parse_node_contents(mpack_tree_t* tree, mpack_node_data_t* node) {
    mpack_assert(tree->parser.state == mpack_tree_parse_state_in_progress);
    mpack_assert(node != NULL, "null node?");

    // read the type. we've already accounted for this byte in
    // possible_nodes_left, so we already know it is in bounds, and we don't
    // need to reserve it for this node.
    mpack_assert(tree->data_length > tree->size);
    uint8_t type = mpack_load_u8(tree->data + tree->size);
    mpack_log("node type %x\n", type);
    tree->parser.current_node_reserved = 0;

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
            return true;

        // negative fixnum
        case 0xe: case 0xf:
            node->type = mpack_type_int;
            node->value.i = (int8_t)type;
            return true;

        // fixmap
        case 0x8:
            node->type = mpack_type_map;
            node->len = (uint32_t)(type & ~0xf0);
            return mpack_tree_parse_children(tree, node);

        // fixarray
        case 0x9:
            node->type = mpack_type_array;
            node->len = (uint32_t)(type & ~0xf0);
            return mpack_tree_parse_children(tree, node);

        // fixstr
        case 0xa: case 0xb:
            node->type = mpack_type_str;
            node->len = (uint32_t)(type & ~0xe0);
            return mpack_tree_parse_bytes(tree, node);

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
            return true;

        // negative fixnum
        case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
        case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
        case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
        case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
            node->type = mpack_type_int;
            node->value.i = (int8_t)type;
            return true;

        // fixmap
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
            node->type = mpack_type_map;
            node->len = (uint32_t)(type & ~0xf0);
            return mpack_tree_parse_children(tree, node);

        // fixarray
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
            node->type = mpack_type_array;
            node->len = (uint32_t)(type & ~0xf0);
            return mpack_tree_parse_children(tree, node);

        // fixstr
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
        case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
        case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
        case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
            node->type = mpack_type_str;
            node->len = (uint32_t)(type & ~0xe0);
            return mpack_tree_parse_bytes(tree, node);
        #endif

        // nil
        case 0xc0:
            node->type = mpack_type_nil;
            return true;

        // bool
        case 0xc2: case 0xc3:
            node->type = mpack_type_bool;
            node->value.b = type & 1;
            return true;

        // bin8
        case 0xc4:
            node->type = mpack_type_bin;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint8_t)))
                return false;
            node->len = mpack_load_u8(tree->data + tree->size + 1);
            return mpack_tree_parse_bytes(tree, node);

        // bin16
        case 0xc5:
            node->type = mpack_type_bin;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->len = mpack_load_u16(tree->data + tree->size + 1);
            return mpack_tree_parse_bytes(tree, node);

        // bin32
        case 0xc6:
            node->type = mpack_type_bin;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->len = mpack_load_u32(tree->data + tree->size + 1);
            return mpack_tree_parse_bytes(tree, node);

        #if MPACK_EXTENSIONS
        // ext8
        case 0xc7:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint8_t)))
                return false;
            node->len = mpack_load_u8(tree->data + tree->size + 1);
            return mpack_tree_parse_ext(tree, node);

        // ext16
        case 0xc8:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->len = mpack_load_u16(tree->data + tree->size + 1);
            return mpack_tree_parse_ext(tree, node);

        // ext32
        case 0xc9:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->len = mpack_load_u32(tree->data + tree->size + 1);
            return mpack_tree_parse_ext(tree, node);
        #endif

        // float
        case 0xca:
            #if MPACK_FLOAT
            if (!mpack_tree_reserve_bytes(tree, sizeof(float)))
                return false;
            node->value.f = mpack_load_float(tree->data + tree->size + 1);
            #else
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->value.f = mpack_load_u32(tree->data + tree->size + 1);
            #endif
            node->type = mpack_type_float;
            return true;

        // double
        case 0xcb:
            #if MPACK_DOUBLE
            if (!mpack_tree_reserve_bytes(tree, sizeof(double)))
                return false;
            node->value.d = mpack_load_double(tree->data + tree->size + 1);
            #else
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint64_t)))
                return false;
            node->value.d = mpack_load_u64(tree->data + tree->size + 1);
            #endif
            node->type = mpack_type_double;
            return true;

        // uint8
        case 0xcc:
            node->type = mpack_type_uint;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint8_t)))
                return false;
            node->value.u = mpack_load_u8(tree->data + tree->size + 1);
            return true;

        // uint16
        case 0xcd:
            node->type = mpack_type_uint;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->value.u = mpack_load_u16(tree->data + tree->size + 1);
            return true;

        // uint32
        case 0xce:
            node->type = mpack_type_uint;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->value.u = mpack_load_u32(tree->data + tree->size + 1);
            return true;

        // uint64
        case 0xcf:
            node->type = mpack_type_uint;
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint64_t)))
                return false;
            node->value.u = mpack_load_u64(tree->data + tree->size + 1);
            return true;

        // int8
        case 0xd0:
            node->type = mpack_type_int;
            if (!mpack_tree_reserve_bytes(tree, sizeof(int8_t)))
                return false;
            node->value.i = mpack_load_i8(tree->data + tree->size + 1);
            return true;

        // int16
        case 0xd1:
            node->type = mpack_type_int;
            if (!mpack_tree_reserve_bytes(tree, sizeof(int16_t)))
                return false;
            node->value.i = mpack_load_i16(tree->data + tree->size + 1);
            return true;

        // int32
        case 0xd2:
            node->type = mpack_type_int;
            if (!mpack_tree_reserve_bytes(tree, sizeof(int32_t)))
                return false;
            node->value.i = mpack_load_i32(tree->data + tree->size + 1);
            return true;

        // int64
        case 0xd3:
            node->type = mpack_type_int;
            if (!mpack_tree_reserve_bytes(tree, sizeof(int64_t)))
                return false;
            node->value.i = mpack_load_i64(tree->data + tree->size + 1);
            return true;

        #if MPACK_EXTENSIONS
        // fixext1
        case 0xd4:
            node->len = 1;
            return mpack_tree_parse_ext(tree, node);

        // fixext2
        case 0xd5:
            node->len = 2;
            return mpack_tree_parse_ext(tree, node);

        // fixext4
        case 0xd6:
            node->len = 4;
            return mpack_tree_parse_ext(tree, node);

        // fixext8
        case 0xd7:
            node->len = 8;
            return mpack_tree_parse_ext(tree, node);

        // fixext16
        case 0xd8:
            node->len = 16;
            return mpack_tree_parse_ext(tree, node);
        #endif

        // str8
        case 0xd9:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint8_t)))
                return false;
            node->len = mpack_load_u8(tree->data + tree->size + 1);
            node->type = mpack_type_str;
            return mpack_tree_parse_bytes(tree, node);

        // str16
        case 0xda:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->len = mpack_load_u16(tree->data + tree->size + 1);
            node->type = mpack_type_str;
            return mpack_tree_parse_bytes(tree, node);

        // str32
        case 0xdb:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->len = mpack_load_u32(tree->data + tree->size + 1);
            node->type = mpack_type_str;
            return mpack_tree_parse_bytes(tree, node);

        // array16
        case 0xdc:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->len = mpack_load_u16(tree->data + tree->size + 1);
            node->type = mpack_type_array;
            return mpack_tree_parse_children(tree, node);

        // array32
        case 0xdd:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->len = mpack_load_u32(tree->data + tree->size + 1);
            node->type = mpack_type_array;
            return mpack_tree_parse_children(tree, node);

        // map16
        case 0xde:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint16_t)))
                return false;
            node->len = mpack_load_u16(tree->data + tree->size + 1);
            node->type = mpack_type_map;
            return mpack_tree_parse_children(tree, node);

        // map32
        case 0xdf:
            if (!mpack_tree_reserve_bytes(tree, sizeof(uint32_t)))
                return false;
            node->len = mpack_load_u32(tree->data + tree->size + 1);
            node->type = mpack_type_map;
            return mpack_tree_parse_children(tree, node);

        // reserved
        case 0xc1:
            mpack_tree_flag_error(tree, mpack_error_invalid);
            return false;

        #if !MPACK_EXTENSIONS
        // ext
        case 0xc7: // fallthrough
        case 0xc8: // fallthrough
        case 0xc9: // fallthrough
        // fixext
        case 0xd4: // fallthrough
        case 0xd5: // fallthrough
        case 0xd6: // fallthrough
        case 0xd7: // fallthrough
        case 0xd8:
            mpack_tree_flag_error(tree, mpack_error_unsupported);
            return false;
        #endif

        #if MPACK_OPTIMIZE_FOR_SIZE
        // any other bytes should have been handled by the infix switch
        default:
            break;
        #endif
    }

    mpack_assert(0, "unreachable");
    return false;
}

static bool mpack_tree_parse_node(mpack_tree_t* tree, mpack_node_data_t* node) {
    mpack_log("parsing a node at position %i in level %i\n",
            (int)tree->size, (int)tree->parser.level);

    if (!mpack_tree_parse_node_contents(tree, node)) {
        mpack_log("node parsing returned false\n");
        return false;
    }

    tree->parser.possible_nodes_left -= tree->parser.current_node_reserved;

    // The reserve for the current node does not include the initial byte
    // previously reserved as part of its parent.
    size_t node_size = tree->parser.current_node_reserved + 1;

    // If the parsed type is a map or array, the reserve includes one byte for
    // each child. We want to subtract these out of possible_nodes_left, but
    // not out of the current size of the tree.
    if (node->type == mpack_type_array)
        node_size -= node->len;
    else if (node->type == mpack_type_map)
        node_size -= node->len * 2;
    tree->size += node_size;

    mpack_log("parsed a node of type %s of %i bytes and "
            "%i additional bytes reserved for children.\n",
            mpack_type_to_string(node->type), (int)node_size,
            (int)tree->parser.current_node_reserved + 1 - (int)node_size);

    return true;
}

/*
 * We read nodes in a loop instead of recursively for maximum performance. The
 * stack holds the amount of children left to read in each level of the tree.
 * Parsing can pause and resume when more data becomes available.
 */
static bool mpack_tree_continue_parsing(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return false;

    mpack_tree_parser_t* parser = &tree->parser;
    mpack_assert(parser->state == mpack_tree_parse_state_in_progress);
    mpack_log("parsing tree elements, %i bytes in buffer\n", (int)tree->data_length);

    // we loop parsing nodes until the parse stack is empty. we break
    // by returning out of the function.
    while (true) {
        mpack_node_data_t* node = parser->stack[parser->level].child;
        size_t level = parser->level;
        if (!mpack_tree_parse_node(tree, node))
            return false;
        --parser->stack[level].left;
        ++parser->stack[level].child;

        mpack_assert(mpack_tree_error(tree) == mpack_ok,
                "mpack_tree_parse_node() should have returned false due to error!");

        // pop empty stack levels, exiting the outer loop when the stack is empty.
        // (we could tail-optimize containers by pre-emptively popping empty
        // stack levels before reading the new element, this way we wouldn't
        // have to loop. but we eventually want to use the parse stack to give
        // better error messages that contain the location of the error, so
        // it needs to be complete.)
        while (parser->stack[parser->level].left == 0) {
            if (parser->level == 0)
                return true;
            --parser->level;
        }
    }
}

static void mpack_tree_cleanup(mpack_tree_t* tree) {
    MPACK_UNUSED(tree);

    #ifdef MPACK_MALLOC
    if (tree->parser.stack_owned) {
        MPACK_FREE(tree->parser.stack);
        tree->parser.stack = NULL;
        tree->parser.stack_owned = false;
    }

    mpack_tree_page_t* page = tree->next;
    while (page != NULL) {
        mpack_tree_page_t* next = page->next;
        mpack_log("freeing page %p\n", (void*)page);
        MPACK_FREE(page);
        page = next;
    }
    tree->next = NULL;
    #endif
}

static bool mpack_tree_parse_start(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return false;

    mpack_tree_parser_t* parser = &tree->parser;
    mpack_assert(parser->state != mpack_tree_parse_state_in_progress,
            "previous parsing was not finished!");

    if (parser->state == mpack_tree_parse_state_parsed)
        mpack_tree_cleanup(tree);

    mpack_log("starting parse\n");
    tree->parser.state = mpack_tree_parse_state_in_progress;
    tree->parser.current_node_reserved = 0;

    // check if we previously parsed a tree
    if (tree->size > 0) {
        #ifdef MPACK_MALLOC
        // if we're buffered, move the remaining data back to the
        // start of the buffer
        // TODO: This is not ideal performance-wise. We should only move data
        // when we need to call the fill function.
        // TODO: We could consider shrinking the buffer here, especially if we
        // determine that the fill function is providing less than a quarter of
        // the buffer size or if messages take up less than a quarter of the
        // buffer size. Maybe this should be configurable.
        if (tree->buffer != NULL) {
            mpack_memmove(tree->buffer, tree->buffer + tree->size, tree->data_length - tree->size);
        }
        else
        #endif
        // otherwise advance past the parsed data
        {
            tree->data += tree->size;
        }
        tree->data_length -= tree->size;
        tree->size = 0;
        tree->node_count = 0;
    }

    // make sure we have at least one byte available before allocating anything
    parser->possible_nodes_left = tree->data_length;
    if (!mpack_tree_reserve_bytes(tree, sizeof(uint8_t))) {
        tree->parser.state = mpack_tree_parse_state_not_started;
        return false;
    }
    mpack_log("parsing tree at %p starting with byte %x\n", tree->data, (uint8_t)tree->data[0]);
    parser->possible_nodes_left -= 1;
    tree->node_count = 1;

    #ifdef MPACK_MALLOC
    parser->stack = parser->stack_local;
    parser->stack_owned = false;
    parser->stack_capacity = sizeof(parser->stack_local) / sizeof(*parser->stack_local);

    if (tree->pool == NULL) {

        // allocate first page
        mpack_tree_page_t* page = (mpack_tree_page_t*)MPACK_MALLOC(MPACK_PAGE_ALLOC_SIZE);
        mpack_log("allocated initial page %p of size %i count %i\n",
                (void*)page, (int)MPACK_PAGE_ALLOC_SIZE, (int)MPACK_NODES_PER_PAGE);
        if (page == NULL) {
            tree->error = mpack_error_memory;
            return false;
        }
        page->next = NULL;
        tree->next = page;

        parser->nodes = page->nodes;
        parser->nodes_left = MPACK_NODES_PER_PAGE;
    }
    else
    #endif
    {
        // otherwise use the provided pool
        mpack_assert(tree->pool != NULL, "no pool provided?");
        parser->nodes = tree->pool;
        parser->nodes_left = tree->pool_count;
    }

    tree->root = parser->nodes;
    ++parser->nodes;
    --parser->nodes_left;

    parser->level = 0;
    parser->stack[0].child = tree->root;
    parser->stack[0].left = 1;

    return true;
}

void mpack_tree_parse(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return;

    if (tree->parser.state != mpack_tree_parse_state_in_progress) {
        if (!mpack_tree_parse_start(tree)) {
            mpack_tree_flag_error(tree, (tree->read_fn == NULL) ?
                    mpack_error_invalid : mpack_error_io);
            return;
        }
    }

    if (!mpack_tree_continue_parsing(tree)) {
        if (mpack_tree_error(tree) != mpack_ok)
            return;

        // We're parsing synchronously on a blocking fill function. If we
        // didn't completely finish parsing the tree, it's an error.
        mpack_log("tree parsing incomplete. flagging error.\n");
        mpack_tree_flag_error(tree, (tree->read_fn == NULL) ?
                mpack_error_invalid : mpack_error_io);
        return;
    }

    mpack_assert(mpack_tree_error(tree) == mpack_ok);
    mpack_assert(tree->parser.level == 0);
    tree->parser.state = mpack_tree_parse_state_parsed;
    mpack_log("parsed tree of %i bytes, %i bytes left\n", (int)tree->size, (int)tree->parser.possible_nodes_left);
    mpack_log("%i nodes in final page\n", (int)tree->parser.nodes_left);
}

bool mpack_tree_try_parse(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return false;

    if (tree->parser.state != mpack_tree_parse_state_in_progress)
        if (!mpack_tree_parse_start(tree))
            return false;

    if (!mpack_tree_continue_parsing(tree))
        return false;

    mpack_assert(mpack_tree_error(tree) == mpack_ok);
    mpack_assert(tree->parser.level == 0);
    tree->parser.state = mpack_tree_parse_state_parsed;
    return true;
}



/*
 * Tree functions
 */

mpack_node_t mpack_tree_root(mpack_tree_t* tree) {
    if (mpack_tree_error(tree) != mpack_ok)
        return mpack_tree_nil_node(tree);

    // We check that a tree was parsed successfully and assert if not. You must
    // call mpack_tree_parse() (or mpack_tree_try_parse() with a success
    // result) in order to access the root node.
    if (tree->parser.state != mpack_tree_parse_state_parsed) {
        mpack_break("Tree has not been parsed! "
                "Did you call mpack_tree_parse() or mpack_tree_try_parse()?");
        mpack_tree_flag_error(tree, mpack_error_bug);
        return mpack_tree_nil_node(tree);
    }

    return mpack_node(tree, tree->root);
}

static void mpack_tree_init_clear(mpack_tree_t* tree) {
    mpack_memset(tree, 0, sizeof(*tree));
    tree->nil_node.type = mpack_type_nil;
    tree->missing_node.type = mpack_type_missing;
    tree->max_size = SIZE_MAX;
    tree->max_nodes = SIZE_MAX;
}

#ifdef MPACK_MALLOC
void mpack_tree_init_data(mpack_tree_t* tree, const char* data, size_t length) {
    mpack_tree_init_clear(tree);

    MPACK_STATIC_ASSERT(MPACK_NODE_PAGE_SIZE >= sizeof(mpack_tree_page_t),
            "MPACK_NODE_PAGE_SIZE is too small");

    MPACK_STATIC_ASSERT(MPACK_PAGE_ALLOC_SIZE <= MPACK_NODE_PAGE_SIZE,
            "incorrect page rounding?");

    tree->data = data;
    tree->data_length = length;
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
    tree->data_length = length;
    tree->pool = node_pool;
    tree->pool_count = node_pool_count;

    mpack_log("===========================\n");
    mpack_log("initializing tree with data of size %i and pool of count %i\n",
            (int)length, (int)node_pool_count);
}

void mpack_tree_init_error(mpack_tree_t* tree, mpack_error_t error) {
    mpack_tree_init_clear(tree);
    tree->error = error;

    mpack_log("===========================\n");
    mpack_log("initializing tree error state %i\n", (int)error);
}

#ifdef MPACK_MALLOC
void mpack_tree_init_stream(mpack_tree_t* tree, mpack_tree_read_t read_fn, void* context,
        size_t max_message_size, size_t max_message_nodes) {
    mpack_tree_init_clear(tree);

    tree->read_fn = read_fn;
    tree->context = context;

    mpack_tree_set_limits(tree, max_message_size, max_message_nodes);
    tree->max_size = max_message_size;
    tree->max_nodes = max_message_nodes;

    mpack_log("===========================\n");
    mpack_log("initializing tree with stream, max size %i max nodes %i\n",
            (int)max_message_size, (int)max_message_nodes);
}
#endif

void mpack_tree_set_limits(mpack_tree_t* tree, size_t max_message_size, size_t max_message_nodes) {
    mpack_assert(max_message_size > 0);
    mpack_assert(max_message_nodes > 0);
    tree->max_size = max_message_size;
    tree->max_nodes = max_message_nodes;
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

static bool mpack_file_tree_read(mpack_tree_t* tree, mpack_file_tree_t* file_tree, FILE* file, size_t max_bytes) {

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
        mpack_tree_init_error(tree, mpack_error_io);
        return false;
    }
    if (size == 0) {
        mpack_tree_init_error(tree, mpack_error_invalid);
        return false;
    }

    // make sure the size is less than max_bytes
    // (this mess exists to safely convert between long and size_t regardless of their widths)
    if (max_bytes != 0 && (((uint64_t)LONG_MAX > (uint64_t)SIZE_MAX && size > (long)SIZE_MAX) || (size_t)size > max_bytes)) {
        mpack_tree_init_error(tree, mpack_error_too_big);
        return false;
    }

    // allocate data
    file_tree->data = (char*)MPACK_MALLOC((size_t)size);
    if (file_tree->data == NULL) {
        mpack_tree_init_error(tree, mpack_error_memory);
        return false;
    }

    // read the file
    long total = 0;
    while (total < size) {
        size_t read = fread(file_tree->data + total, 1, (size_t)(size - total), file);
        if (read <= 0) {
            mpack_tree_init_error(tree, mpack_error_io);
            MPACK_FREE(file_tree->data);
            return false;
        }
        total += (long)read;
    }

    file_tree->size = (size_t)size;
    return true;
}

static bool mpack_tree_file_check_max_bytes(mpack_tree_t* tree, size_t max_bytes) {

    // the C STDIO family of file functions use long (e.g. ftell)
    if (max_bytes > LONG_MAX) {
        mpack_break("max_bytes of %" PRIu64 " is invalid, maximum is LONG_MAX", (uint64_t)max_bytes);
        mpack_tree_init_error(tree, mpack_error_bug);
        return false;
    }

    return true;
}

static void mpack_tree_init_stdfile_noclose(mpack_tree_t* tree, FILE* stdfile, size_t max_bytes) {

    // allocate file tree
    mpack_file_tree_t* file_tree = (mpack_file_tree_t*) MPACK_MALLOC(sizeof(mpack_file_tree_t));
    if (file_tree == NULL) {
        mpack_tree_init_error(tree, mpack_error_memory);
        return;
    }

    // read all data
    if (!mpack_file_tree_read(tree, file_tree, stdfile, max_bytes)) {
        MPACK_FREE(file_tree);
        return;
    }

    mpack_tree_init_data(tree, file_tree->data, file_tree->size);
    mpack_tree_set_context(tree, file_tree);
    mpack_tree_set_teardown(tree, mpack_file_tree_teardown);
}

void mpack_tree_init_stdfile(mpack_tree_t* tree, FILE* stdfile, size_t max_bytes, bool close_when_done) {
    if (!mpack_tree_file_check_max_bytes(tree, max_bytes))
        return;

    mpack_tree_init_stdfile_noclose(tree, stdfile, max_bytes);

    if (close_when_done)
        fclose(stdfile);
}

void mpack_tree_init_filename(mpack_tree_t* tree, const char* filename, size_t max_bytes) {
    if (!mpack_tree_file_check_max_bytes(tree, max_bytes))
        return;

    // open the file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        mpack_tree_init_error(tree, mpack_error_io);
        return;
    }

    mpack_tree_init_stdfile(tree, file, max_bytes, true);
}
#endif

mpack_error_t mpack_tree_destroy(mpack_tree_t* tree) {
    mpack_tree_cleanup(tree);

    #ifdef MPACK_MALLOC
    if (tree->buffer)
        MPACK_FREE(tree->buffer);
    #endif

    if (tree->teardown)
        tree->teardown(tree);
    tree->teardown = NULL;

    return tree->error;
}

void mpack_tree_flag_error(mpack_tree_t* tree, mpack_error_t error) {
    if (tree->error == mpack_ok) {
        mpack_log("tree %p setting error %i: %s\n", (void*)tree, (int)error, mpack_error_to_string(error));
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

    mpack_tag_t tag = MPACK_TAG_ZERO;

    tag.type = node.data->type;
    switch (node.data->type) {
        case mpack_type_missing:
            // If a node is missing, I don't know if it makes sense to ask for
            // a tag for it. We'll return a missing tag to match the missing
            // node I guess, but attempting to use the tag for anything (like
            // writing it for example) will flag mpack_error_bug.
            break;
        case mpack_type_nil:                                            break;
        case mpack_type_bool:    tag.v.b = node.data->value.b;          break;
        case mpack_type_float:   tag.v.f = node.data->value.f;          break;
        case mpack_type_double:  tag.v.d = node.data->value.d;          break;
        case mpack_type_int:     tag.v.i = node.data->value.i;          break;
        case mpack_type_uint:    tag.v.u = node.data->value.u;          break;

        case mpack_type_str:     tag.v.l = node.data->len;     break;
        case mpack_type_bin:     tag.v.l = node.data->len;     break;

        #if MPACK_EXTENSIONS
        case mpack_type_ext:
            tag.v.l = node.data->len;
            tag.exttype = mpack_node_exttype_unchecked(node);
            break;
        #endif

        case mpack_type_array:   tag.v.n = node.data->len;  break;
        case mpack_type_map:     tag.v.n = node.data->len;  break;

        default:
            mpack_assert(0, "unrecognized type %i", (int)node.data->type);
            break;
    }
    return tag;
}

#if MPACK_DEBUG && MPACK_STDIO
static void mpack_node_print_element(mpack_node_t node, mpack_print_t* print, size_t depth) {
    mpack_node_data_t* data = node.data;
    size_t i,j;
    switch (data->type) {
        case mpack_type_str:
            {
                mpack_print_append_cstr(print, "\"");
                const char* bytes = mpack_node_data_unchecked(node);
                for (i = 0; i < data->len; ++i) {
                    char c = bytes[i];
                    switch (c) {
                        case '\n': mpack_print_append_cstr(print, "\\n"); break;
                        case '\\': mpack_print_append_cstr(print, "\\\\"); break;
                        case '"': mpack_print_append_cstr(print, "\\\""); break;
                        default: mpack_print_append(print, &c, 1); break;
                    }
                }
                mpack_print_append_cstr(print, "\"");
            }
            break;

        case mpack_type_array:
            mpack_print_append_cstr(print, "[\n");
            for (i = 0; i < data->len; ++i) {
                for (j = 0; j < depth + 1; ++j)
                    mpack_print_append_cstr(print, "    ");
                mpack_node_print_element(mpack_node_array_at(node, i), print, depth + 1);
                if (i != data->len - 1)
                    mpack_print_append_cstr(print, ",");
                mpack_print_append_cstr(print, "\n");
            }
            for (i = 0; i < depth; ++i)
                mpack_print_append_cstr(print, "    ");
            mpack_print_append_cstr(print, "]");
            break;

        case mpack_type_map:
            mpack_print_append_cstr(print, "{\n");
            for (i = 0; i < data->len; ++i) {
                for (j = 0; j < depth + 1; ++j)
                    mpack_print_append_cstr(print, "    ");
                mpack_node_print_element(mpack_node_map_key_at(node, i), print, depth + 1);
                mpack_print_append_cstr(print, ": ");
                mpack_node_print_element(mpack_node_map_value_at(node, i), print, depth + 1);
                if (i != data->len - 1)
                    mpack_print_append_cstr(print, ",");
                mpack_print_append_cstr(print, "\n");
            }
            for (i = 0; i < depth; ++i)
                mpack_print_append_cstr(print, "    ");
            mpack_print_append_cstr(print, "}");
            break;

        default:
            {
                const char* prefix = NULL;
                size_t prefix_length = 0;
                if (mpack_node_type(node) == mpack_type_bin
                        #if MPACK_EXTENSIONS
                        || mpack_node_type(node) == mpack_type_ext
                        #endif
                ) {
                    prefix = mpack_node_data(node);
                    prefix_length = mpack_node_data_len(node);
                }

                char buf[256];
                mpack_tag_t tag = mpack_node_tag(node);
                mpack_tag_debug_pseudo_json(tag, buf, sizeof(buf), prefix, prefix_length);
                mpack_print_append_cstr(print, buf);
            }
            break;
    }
}

void mpack_node_print_to_buffer(mpack_node_t node, char* buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        mpack_assert(false, "buffer size is zero!");
        return;
    }

    mpack_print_t print;
    mpack_memset(&print, 0, sizeof(print));
    print.buffer = buffer;
    print.size = buffer_size;
    mpack_node_print_element(node, &print, 0);
    mpack_print_append(&print, "",  1); // null-terminator
    mpack_print_flush(&print);

    // we always make sure there's a null-terminator at the end of the buffer
    // in case we ran out of space.
    print.buffer[print.size - 1] = '\0';
}

void mpack_node_print_to_callback(mpack_node_t node, mpack_print_callback_t callback, void* context) {
    char buffer[1024];
    mpack_print_t print;
    mpack_memset(&print, 0, sizeof(print));
    print.buffer = buffer;
    print.size = sizeof(buffer);
    print.callback = callback;
    print.context = context;
    mpack_node_print_element(node, &print, 0);
    mpack_print_flush(&print);
}

void mpack_node_print_to_file(mpack_node_t node, FILE* file) {
    mpack_assert(file != NULL, "file is NULL");

    char buffer[1024];
    mpack_print_t print;
    mpack_memset(&print, 0, sizeof(print));
    print.buffer = buffer;
    print.size = sizeof(buffer);
    print.callback = &mpack_print_file_callback;
    print.context = file;

    size_t depth = 2;
    size_t i;
    for (i = 0; i < depth; ++i)
        mpack_print_append_cstr(&print, "    ");
    mpack_node_print_element(node, &print, depth);
    mpack_print_append_cstr(&print, "\n");
    mpack_print_flush(&print);
}
#endif



/*
 * Node Value Functions
 */

#if MPACK_EXTENSIONS
mpack_timestamp_t mpack_node_timestamp(mpack_node_t node) {
    mpack_timestamp_t timestamp = {0, 0};

    // we'll let mpack_node_exttype() do most checks
    if (mpack_node_exttype(node) != MPACK_EXTTYPE_TIMESTAMP) {
        mpack_log("exttype %i\n", mpack_node_exttype(node));
        mpack_node_flag_error(node, mpack_error_type);
        return timestamp;
    }

    const char* p = mpack_node_data_unchecked(node);

    switch (node.data->len) {
        case 4:
            timestamp.nanoseconds = 0;
            timestamp.seconds = mpack_load_u32(p);
            break;

        case 8: {
            uint64_t value = mpack_load_u64(p);
            timestamp.nanoseconds = (uint32_t)(value >> 34);
            timestamp.seconds = value & ((MPACK_UINT64_C(1) << 34) - 1);
            break;
        }

        case 12:
            timestamp.nanoseconds = mpack_load_u32(p);
            timestamp.seconds = mpack_load_i64(p + 4);
            break;

        default:
            mpack_tree_flag_error(node.tree, mpack_error_invalid);
            return timestamp;
    }

    if (timestamp.nanoseconds > MPACK_TIMESTAMP_NANOSECONDS_MAX) {
        mpack_tree_flag_error(node.tree, mpack_error_invalid);
        mpack_timestamp_t zero = {0, 0};
        return zero;
    }

    return timestamp;
}

int64_t mpack_node_timestamp_seconds(mpack_node_t node) {
    return mpack_node_timestamp(node).seconds;
}

uint32_t mpack_node_timestamp_nanoseconds(mpack_node_t node) {
    return mpack_node_timestamp(node).nanoseconds;
}
#endif



/*
 * Node Data Functions
 */

void mpack_node_check_utf8(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    mpack_node_data_t* data = node.data;
    if (data->type != mpack_type_str || !mpack_utf8_check(mpack_node_data_unchecked(node), data->len))
        mpack_node_flag_error(node, mpack_error_type);
}

void mpack_node_check_utf8_cstr(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    mpack_node_data_t* data = node.data;
    if (data->type != mpack_type_str || !mpack_utf8_check_no_null(mpack_node_data_unchecked(node), data->len))
        mpack_node_flag_error(node, mpack_error_type);
}

size_t mpack_node_copy_data(mpack_node_t node, char* buffer, size_t bufsize) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_assert(bufsize == 0 || buffer != NULL, "buffer is NULL for maximum of %i bytes", (int)bufsize);

    mpack_type_t type = node.data->type;
    if (type != mpack_type_str && type != mpack_type_bin
            #if MPACK_EXTENSIONS
            && type != mpack_type_ext
            #endif
    ) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    if (node.data->len > bufsize) {
        mpack_node_flag_error(node, mpack_error_too_big);
        return 0;
    }

    mpack_memcpy(buffer, mpack_node_data_unchecked(node), node.data->len);
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

    if (!mpack_utf8_check(mpack_node_data_unchecked(node), node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    mpack_memcpy(buffer, mpack_node_data_unchecked(node), node.data->len);
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

    if (!mpack_str_check_no_null(mpack_node_data_unchecked(node), node.data->len)) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    mpack_memcpy(buffer, mpack_node_data_unchecked(node), node.data->len);
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

    if (!mpack_utf8_check_no_null(mpack_node_data_unchecked(node), node.data->len)) {
        buffer[0] = '\0';
        mpack_node_flag_error(node, mpack_error_type);
        return;
    }

    mpack_memcpy(buffer, mpack_node_data_unchecked(node), node.data->len);
    buffer[node.data->len] = '\0';
}

#ifdef MPACK_MALLOC
char* mpack_node_data_alloc(mpack_node_t node, size_t maxlen) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    // make sure this is a valid data type
    mpack_type_t type = node.data->type;
    if (type != mpack_type_str && type != mpack_type_bin
            #if MPACK_EXTENSIONS
            && type != mpack_type_ext
            #endif
    ) {
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

    mpack_memcpy(ret, mpack_node_data_unchecked(node), node.data->len);
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

    if (!mpack_str_check_no_null(mpack_node_data_unchecked(node), node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node.data->len + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, mpack_node_data_unchecked(node), node.data->len);
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

    if (!mpack_utf8_check_no_null(mpack_node_data_unchecked(node), node.data->len)) {
        mpack_node_flag_error(node, mpack_error_type);
        return NULL;
    }

    char* ret = (char*) MPACK_MALLOC((size_t)(node.data->len + 1));
    if (ret == NULL) {
        mpack_node_flag_error(node, mpack_error_memory);
        return NULL;
    }

    mpack_memcpy(ret, mpack_node_data_unchecked(node), node.data->len);
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

    size_t i;
    for (i = 0; i < node.data->len; ++i) {
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

    size_t i;
    for (i = 0; i < node.data->len; ++i) {
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

    mpack_tree_t* tree = node.tree;
    mpack_node_data_t* found = NULL;

    size_t i;
    for (i = 0; i < node.data->len; ++i) {
        mpack_node_data_t* key = mpack_node_child(node, i * 2);

        if (key->type == mpack_type_str && key->len == length &&
                mpack_memcmp(str, mpack_node_data_unchecked(mpack_node(tree, key)), length) == 0) {
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
    if (!data) {
        if (tree->error == mpack_ok)
            return mpack_tree_missing_node(tree);
        return mpack_tree_nil_node(tree);
    }
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
    size_t i;
    for (i = 0; i < count; ++i) {
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

mpack_type_t mpack_node_type(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_type_nil;
    return node.data->type;
}

bool mpack_node_is_nil(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok) {
        // All nodes are treated as nil nodes when we are in error.
        return true;
    }
    return node.data->type == mpack_type_nil;
}

bool mpack_node_is_missing(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok) {
        // errors still return nil nodes, not missing nodes.
        return false;
    }
    return node.data->type == mpack_type_missing;
}

void mpack_node_nil(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    if (node.data->type != mpack_type_nil)
        mpack_node_flag_error(node, mpack_error_type);
}

void mpack_node_missing(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return;
    if (node.data->type != mpack_type_missing)
        mpack_node_flag_error(node, mpack_error_type);
}

bool mpack_node_bool(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return false;

    if (node.data->type == mpack_type_bool)
        return node.data->value.b;

    mpack_node_flag_error(node, mpack_error_type);
    return false;
}

void mpack_node_true(mpack_node_t node) {
    if (mpack_node_bool(node) != true)
        mpack_node_flag_error(node, mpack_error_type);
}

void mpack_node_false(mpack_node_t node) {
    if (mpack_node_bool(node) != false)
        mpack_node_flag_error(node, mpack_error_type);
}

uint8_t mpack_node_u8(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_UINT8_MAX)
            return (uint8_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= 0 && node.data->value.i <= MPACK_UINT8_MAX)
            return (uint8_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int8_t mpack_node_i8(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_INT8_MAX)
            return (int8_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= MPACK_INT8_MIN && node.data->value.i <= MPACK_INT8_MAX)
            return (int8_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint16_t mpack_node_u16(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_UINT16_MAX)
            return (uint16_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= 0 && node.data->value.i <= MPACK_UINT16_MAX)
            return (uint16_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int16_t mpack_node_i16(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_INT16_MAX)
            return (int16_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= MPACK_INT16_MIN && node.data->value.i <= MPACK_INT16_MAX)
            return (int16_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint32_t mpack_node_u32(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_UINT32_MAX)
            return (uint32_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= 0 && node.data->value.i <= MPACK_UINT32_MAX)
            return (uint32_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int32_t mpack_node_i32(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= MPACK_INT32_MAX)
            return (int32_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= MPACK_INT32_MIN && node.data->value.i <= MPACK_INT32_MAX)
            return (int32_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

uint64_t mpack_node_u64(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        return node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        if (node.data->value.i >= 0)
            return (uint64_t)node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int64_t mpack_node_i64(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_uint) {
        if (node.data->value.u <= (uint64_t)MPACK_INT64_MAX)
            return (int64_t)node.data->value.u;
    } else if (node.data->type == mpack_type_int) {
        return node.data->value.i;
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

unsigned int mpack_node_uint(mpack_node_t node) {

    // This should be true at compile-time, so this just wraps the 32-bit function.
    if (sizeof(unsigned int) == 4)
        return (unsigned int)mpack_node_u32(node);

    // Otherwise we use u64 and check the range.
    uint64_t val = mpack_node_u64(node);
    if (val <= MPACK_UINT_MAX)
        return (unsigned int)val;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

int mpack_node_int(mpack_node_t node) {

    // This should be true at compile-time, so this just wraps the 32-bit function.
    if (sizeof(int) == 4)
        return (int)mpack_node_i32(node);

    // Otherwise we use i64 and check the range.
    int64_t val = mpack_node_i64(node);
    if (val >= MPACK_INT_MIN && val <= MPACK_INT_MAX)
        return (int)val;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

#if MPACK_FLOAT
float mpack_node_float(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0f;

    if (node.data->type == mpack_type_uint)
        return (float)node.data->value.u;
    if (node.data->type == mpack_type_int)
        return (float)node.data->value.i;
    if (node.data->type == mpack_type_float)
        return node.data->value.f;

    if (node.data->type == mpack_type_double) {
        #if MPACK_DOUBLE
        return (float)node.data->value.d;
        #else
        return mpack_shorten_raw_double_to_float(node.data->value.d);
        #endif
    }

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0f;
}
#endif

#if MPACK_DOUBLE
double mpack_node_double(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0;

    if (node.data->type == mpack_type_uint)
        return (double)node.data->value.u;
    else if (node.data->type == mpack_type_int)
        return (double)node.data->value.i;
    else if (node.data->type == mpack_type_float)
        return (double)node.data->value.f;
    else if (node.data->type == mpack_type_double)
        return node.data->value.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0;
}
#endif

#if MPACK_FLOAT
float mpack_node_float_strict(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0f;

    if (node.data->type == mpack_type_float)
        return node.data->value.f;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0f;
}
#endif

#if MPACK_DOUBLE
double mpack_node_double_strict(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0.0;

    if (node.data->type == mpack_type_float)
        return (double)node.data->value.f;
    else if (node.data->type == mpack_type_double)
        return node.data->value.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0.0;
}
#endif

#if !MPACK_FLOAT
uint32_t mpack_node_raw_float(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_float)
        return node.data->value.f;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}
#endif

#if !MPACK_DOUBLE
uint64_t mpack_node_raw_double(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_double)
        return node.data->value.d;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}
#endif

#if MPACK_EXTENSIONS
int8_t mpack_node_exttype(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_ext)
        return mpack_node_exttype_unchecked(node);

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}
#endif

uint32_t mpack_node_data_len(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    mpack_type_t type = node.data->type;
    if (type == mpack_type_str || type == mpack_type_bin
            #if MPACK_EXTENSIONS
            || type == mpack_type_ext
            #endif
            )
        return (uint32_t)node.data->len;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

size_t mpack_node_strlen(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_str)
        return (size_t)node.data->len;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

const char* mpack_node_str(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    mpack_type_t type = node.data->type;
    if (type == mpack_type_str)
        return mpack_node_data_unchecked(node);

    mpack_node_flag_error(node, mpack_error_type);
    return NULL;
}

const char* mpack_node_data(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    mpack_type_t type = node.data->type;
    if (type == mpack_type_str || type == mpack_type_bin
            #if MPACK_EXTENSIONS
            || type == mpack_type_ext
            #endif
            )
        return mpack_node_data_unchecked(node);

    mpack_node_flag_error(node, mpack_error_type);
    return NULL;
}

const char* mpack_node_bin_data(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return NULL;

    if (node.data->type == mpack_type_bin)
        return mpack_node_data_unchecked(node);

    mpack_node_flag_error(node, mpack_error_type);
    return NULL;
}

size_t mpack_node_bin_size(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type == mpack_type_bin)
        return (size_t)node.data->len;

    mpack_node_flag_error(node, mpack_error_type);
    return 0;
}

size_t mpack_node_array_length(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return (size_t)node.data->len;
}

mpack_node_t mpack_node_array_at(mpack_node_t node, size_t index) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tree_nil_node(node.tree);

    if (node.data->type != mpack_type_array) {
        mpack_node_flag_error(node, mpack_error_type);
        return mpack_tree_nil_node(node.tree);
    }

    if (index >= node.data->len) {
        mpack_node_flag_error(node, mpack_error_data);
        return mpack_tree_nil_node(node.tree);
    }

    return mpack_node(node.tree, mpack_node_child(node, index));
}

size_t mpack_node_map_count(mpack_node_t node) {
    if (mpack_node_error(node) != mpack_ok)
        return 0;

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return 0;
    }

    return node.data->len;
}

// internal node map lookup
static mpack_node_t mpack_node_map_at(mpack_node_t node, size_t index, size_t offset) {
    if (mpack_node_error(node) != mpack_ok)
        return mpack_tree_nil_node(node.tree);

    if (node.data->type != mpack_type_map) {
        mpack_node_flag_error(node, mpack_error_type);
        return mpack_tree_nil_node(node.tree);
    }

    if (index >= node.data->len) {
        mpack_node_flag_error(node, mpack_error_data);
        return mpack_tree_nil_node(node.tree);
    }

    return mpack_node(node.tree, mpack_node_child(node, index * 2 + offset));
}

mpack_node_t mpack_node_map_key_at(mpack_node_t node, size_t index) {
    return mpack_node_map_at(node, index, 0);
}

mpack_node_t mpack_node_map_value_at(mpack_node_t node, size_t index) {
    return mpack_node_map_at(node, index, 1);
}

#endif

MPACK_SILENCE_WARNINGS_END
