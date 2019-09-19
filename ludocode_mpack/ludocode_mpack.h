#ifndef LUDOCODE_MPACK_H
#define LUDOCODE_MPACK_H

/** The block below describes the properties of this JUCE module,
    and is read by the Projucer to automatically generate project code that uses it.

    BEGIN_JUCE_MODULE_DECLARATION

    ID:             ludocode_mpack
    vendor:         Ludocode
    version:        1.0.0
    name:           mpack
    description:    MPack is a C implementation of an encoder and decoder for the MessagePack serialization format.
    website:        https://github.com/ludocode/mpack
    license:        MIT

    END_JUCE_MODULE_DECLARATION
*/

//==============================================================================
/** Config: MPACK_READER

    Enables compilation of the base Tag Reader.
*/
#ifndef MPACK_READER
    #define MPACK_READER 1
#endif

/** Config: MPACK_EXPECT

    Enables compilation of the static Expect API.
*/
#ifndef MPACK_EXPECT
    #define MPACK_EXPECT 1
#endif

/** Config: MPACK_NODE

    Enables compilation of the dynamic Node API.
*/
#ifndef MPACK_NODE
    #define MPACK_NODE 1
#endif

/** Config: MPACK_WRITER

    Enables compilation of the Writer.
*/
#ifndef MPACK_WRITER
    #define MPACK_WRITER 1
#endif

/** Config: MPACK_COMPATIBILITY

    Enables compatibility features for reading and writing older
    versions of MessagePack.

    This is disabled by default. When disabled, the behaviour is equivalent to
    using the default version, @ref mpack_version_current.

    Enable this if you need to interoperate with applications or data that do
    not support the new (v5) MessagePack spec. See the section on v4
    compatibility in @ref docs/protocol.md for more information.
*/
#ifndef MPACK_COMPATIBILITY
    #define MPACK_COMPATIBILITY 0
#endif

/** Config: MPACK_EXTENSIONS

    Enables the use of extension types.

    This is disabled by default.

    If disabled, functions to read and write extensions will not exist,
    and any occurrence of extension types in parsed messages will flag @ref mpack_error_invalid.

    MPack discourages the use of extension types. See the section on extension
    types in @ref docs/protocol.md for more information.
*/
#ifndef MPACK_EXTENSIONS
    #define MPACK_EXTENSIONS 0
#endif

//==============================================================================
// NB: This is unfortunately needed to allow access to a bunch of code in a unity-build module!
#undef MPACK_INTERNAL
#define MPACK_INTERNAL 1

//==============================================================================
#include "../src/mpack/mpack-defaults.h"
#include "../src/mpack/mpack-common.h"
#include "../src/mpack/mpack-expect.h"
#include "../src/mpack/mpack-node.h"
#include "../src/mpack/mpack-platform.h"
#include "../src/mpack/mpack-reader.h"
#include "../src/mpack/mpack-writer.h"

#endif // LUDOCODE_MPACK_H
