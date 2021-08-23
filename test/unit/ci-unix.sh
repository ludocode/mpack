#!/bin/bash

# Builds and runs the unit test suite under UNIX.
#
# Pass environment variable CC to specify the compiler.
#
# This script is run by the continuous integration server to test MPack on UNIX
# systems.

set -e

# Amalgamate if necessary
if [[ "$AMALGAMATED" == "1" ]]; then
    tools/amalgamate.sh
    cd .build/amalgamation
fi
pwd

# Run the "more" variant of unit tests
tools/unit.sh more
