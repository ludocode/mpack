import platform, os


# Common environment setup

env = Environment()
env['ENV']['TERM'] = os.environ['TERM']
if os.environ.has_key('CC'):
    env['CC'] = os.environ['CC']

env.Append(CPPFLAGS = [
    "-Wall", "-Wextra", "-Werror",
    "-Wconversion", "-Wno-sign-conversion", "-Wno-float-conversion",
    "-fprofile-arcs", "-ftest-coverage",
    "-Isrc", "-Itest",
    "-DMPACK_SCONS=1",
    "-g",
    ])
env.Append(LINKFLAGS = [
    "-g",
    "-fprofile-arcs", "-ftest-coverage"
    ])


# Optional flags used in various builds

if ARGUMENTS.get('dev'):
    env.Append(CPPFLAGS = ["-DMPACK_DEV=1"])

allfeatures = [
    "-DMPACK_READER=1",
    "-DMPACK_WRITER=1",
    "-DMPACK_EXPECT=1",
    "-DMPACK_NODE=1",
]
noioconfigs = [
    "-DMPACK_STDLIB=1",
    "-DMPACK_SETJMP=1",
    "-DMPACK_MALLOC=test_malloc",
    "-DMPACK_FREE=test_free",
]
allconfigs = noioconfigs + ["-DMPACK_STDIO=1"]

debugflags = ["-DDEBUG", "-O0"]
releaseflags = ["-Os"]
cflags = ["-std=c99", "-Wc++-compat"]
cxxflags = ["-xc++", "-std=c++98"]


# Functions to add a variant build. One variant build will build and run the
# entire library and test suite in a given configuration.

def AddBuild(variant_dir, cppflags, linkflags):
    env.SConscript("SConscript",
            variant_dir="build/" + variant_dir,
            src="../..",
            exports={'env': env, 'CPPFLAGS': cppflags, 'LINKFLAGS': linkflags},
            duplicate=0)

def AddBuilds(variant_dir, cppflags, linkflags):
    AddBuild("debug-" + variant_dir, debugflags + cppflags, debugflags + linkflags)
    AddBuild("release-" + variant_dir, releaseflags + cppflags, releaseflags + linkflags)


# The default build, everything in debug. Run "scons dev=1" during
# development to build only this. You would also use this for static
# analysis, e.g. "scan-build scons dev=1".

AddBuild("debug", allfeatures + allconfigs + debugflags + cflags, [])


# Otherwise run all builds. Use a parallel build, e.g. "scons -j12" to
# build this in a reasonable amount of time.

if not ARGUMENTS.get('dev'):
    AddBuild("release", allfeatures + allconfigs + releaseflags + cflags, [])

    # feature subsets with default configuration
    AddBuilds("empty", allconfigs + cflags, [])
    AddBuilds("writer", ["-DMPACK_WRITER=1"] + allconfigs + cflags, [])
    AddBuilds("reader", ["-DMPACK_READER=1"] + allconfigs + cflags, [])
    AddBuilds("expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + allconfigs + cflags, [])
    AddBuilds("node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + allconfigs + cflags, [])

    # no i/o
    AddBuilds("noio", allfeatures + noioconfigs + cflags, [])
    AddBuilds("noio-writer", ["-DMPACK_WRITER=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-reader", ["-DMPACK_READER=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + noioconfigs + cflags, [])

    # embedded builds without libc
    AddBuilds("embed", allfeatures + cflags, [])
    AddBuilds("embed-writer", ["-DMPACK_WRITER=1"] + cflags, [])
    AddBuilds("embed-reader", ["-DMPACK_READER=1"] + cflags, [])
    AddBuilds("embed-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + cflags, [])
    AddBuilds("embed-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + cflags, [])

    # miscellaneous test builds
    AddBuilds("cxx", allfeatures + allconfigs + cxxflags, ["-lstdc++"])
    AddBuilds("notrack", ["-DMPACK_NO_TRACKING=1"] + allfeatures + allconfigs + cflags, [])

    # 32-bit builds
    if '64' in platform.architecture()[0]:
        AddBuilds("32",     allfeatures + allconfigs + cflags + ["-m32"], ["-m32"])
        AddBuilds("cxx-32", allfeatures + allconfigs + cxxflags + ["-m32"], ["-m32", "-lstdc++"])
