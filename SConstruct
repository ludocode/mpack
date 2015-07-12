import platform, os


# Common environment setup

env = Environment()
for x in os.environ.keys():
    if x in ["CC", "CXX", "PATH", "TRAVIS", "TERM"] or x.startswith("CLANG_") or x.startswith("CCC_"):
        env[x] = os.environ[x]
        env["ENV"][x] = os.environ[x]

env.Append(CPPFLAGS = [
    "-Wall", "-Wextra", "-Werror",
    "-Wconversion", "-Wno-sign-conversion",
    "-Isrc", "-Itest",
    "-DMPACK_SCONS=1",
    "-g",
    ])
env.Append(LINKFLAGS = [
    "-g",
    ])

if 'CC' not in env or "clang" not in env['CC']:
    env.Append(CPPFLAGS = ["-Wno-float-conversion"]) # unsupported in clang 3.4
    env.Append(CPPFLAGS = ["-fprofile-arcs", "-ftest-coverage"]) # only works properly with gcc
    env.Append(LINKFLAGS = ["-fprofile-arcs", "-ftest-coverage"])


# Optional flags used in various builds

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

if ARGUMENTS.get('all'):
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
    AddBuilds("realloc", allfeatures + allconfigs + debugflags + cflags + ["-DMPACK_REALLOC=test_realloc"], [])

    # 32-bit builds (note Travis-CI doesn't support multilib)
    if 'TRAVIS' not in env and '64' in platform.architecture()[0]:
        AddBuilds("32",     allfeatures + allconfigs + cflags + ["-m32"], ["-m32"])
        AddBuilds("cxx-32", allfeatures + allconfigs + cxxflags + ["-m32"], ["-m32", "-lstdc++"])
