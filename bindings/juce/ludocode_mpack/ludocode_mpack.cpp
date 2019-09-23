#include "ludocode_mpack.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127 4310)
#endif

#undef MPACK_INTERNAL
#include "../../../src/mpack/mpack-common.c"
#undef MPACK_INTERNAL
#include "../../../src/mpack/mpack-expect.c"
#undef MPACK_INTERNAL
#include "../../../src/mpack/mpack-node.c"
#undef MPACK_INTERNAL
#undef MPACK_EMIT_INLINE_DEFS
#include "../../../src/mpack/mpack-platform.c"
#undef MPACK_INTERNAL
#include "../../../src/mpack/mpack-reader.c"
#undef MPACK_INTERNAL
#include "../../../src/mpack/mpack-writer.c"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
