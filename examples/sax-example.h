#include "mpack/mpack.h"

typedef struct sax_callbacks_t {
    void (*nil_element)(void* context);
    void (*bool_element)(void* context, int64_t value);
    void (*int_element)(void* context, int64_t value);
    void (*uint_element)(void* context, uint64_t value);
    void (*string_element)(void* context, const char* data, uint32_t length);

    void (*start_map)(void* context, uint32_t pair_count);
    void (*start_array)(void* context, uint32_t element_count);
    void (*finish_map)(void* context);
    void (*finish_array)(void* context);
} sax_callbacks_t;

/**
 * Parse a blob of MessagePack data, calling the appropriate callback for each
 * element encountered.
 *
 * @return true if successful, false if any error occurs.
 */
bool parse_messagepack(const char* data, size_t length,
        const sax_callbacks_t* callbacks, void* context);
