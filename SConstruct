import platform, os

env = Environment()
env['ENV']['TERM'] = os.environ['TERM']
if os.environ.has_key('CC'):
    env['CC'] = os.environ['CC']

env.Append(CPPFLAGS = [
    "-Wall", "-Wextra", "-Werror",
    "-Wconversion", "-Wno-sign-conversion", "-Wno-float-conversion",
    "-fprofile-arcs", "-ftest-coverage",
    "-Isrc", "-Itest",
    "-g",
    ])
env.Append(LINKFLAGS = [
    "-g",
    "-fprofile-arcs", "-ftest-coverage"
    ])

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

def AddBuild(variant_dir, cppflags, linkflags):
    env.SConscript("SConscript",
            variant_dir=variant_dir,
            src="../..",
            exports={'env': env, 'CPPFLAGS': cppflags, 'LINKFLAGS': linkflags},
            duplicate=0)

# The default build, everything in debug. run "scons dev=1" during
# development to build only this. You would also use this for static
# analysis, e.g. "scan-build scons dev=1".
AddBuild("build/debug", allfeatures + allconfigs + debugflags + cflags, [])

# run parallel build, e.g. "scons -j16" to build this in a reasonable time
if not ARGUMENTS.get('dev'):

    # debug build with subset of features
    AddBuild("build/debug-empty", allconfigs + debugflags + cflags, [])
    AddBuild("build/debug-writer", ["-DMPACK_WRITER=1"] + allconfigs + debugflags + cflags, [])
    AddBuild("build/debug-reader", ["-DMPACK_READER=1"] + allconfigs + debugflags + cflags, [])
    AddBuild("build/debug-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + allconfigs + debugflags + cflags, [])
    AddBuild("build/debug-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + allconfigs + debugflags + cflags, [])

    # debug build without i/o
    AddBuild("build/debug-noio", allfeatures + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-noio-writer", ["-DMPACK_WRITER=1"] + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-noio-reader", ["-DMPACK_READER=1"] + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-noio-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-noio-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + noioconfigs + debugflags + cflags, [])

    # debug build without libc
    AddBuild("build/debug-embed", allfeatures + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-embed-writer", ["-DMPACK_WRITER=1"] + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-embed-reader", ["-DMPACK_READER=1"] + noioconfigs + debugflags + cflags, [])
    AddBuild("build/debug-embed-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + noioconfigs + debugflags + cflags, [])
    # node currently requires malloc()
    #AddBuild("build/debug-embed-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + noioconfigs + debugflags + cflags, [])

    # release builds
    AddBuild("build/release", allfeatures + allconfigs + releaseflags + cflags, [])
    AddBuild("build/release-writer", ["-DMPACK_WRITER=1"] + allconfigs + releaseflags + cflags, [])
    AddBuild("build/release-reader", ["-DMPACK_READER=1"] + allconfigs + releaseflags + cflags, [])
    AddBuild("build/release-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + allconfigs + releaseflags + cflags, [])
    AddBuild("build/release-node", ["-DMPACK_READER=1", "-DMPACK_NODE=1"] + allconfigs + releaseflags + cflags, [])

    # miscellaneous test builds
    AddBuild("build/debug-cxx", allfeatures + allconfigs + debugflags + cxxflags, ["-lstdc++"])
    AddBuild("build/debug-notrack", allfeatures + allconfigs + ["-DMPACK_DEBUG=1", "-O0"] + cflags, [])

    # 32-bit builds
    if '64' in platform.architecture()[0]:
        AddBuild("build/debug-32",   allfeatures + allconfigs + debugflags + cflags + ["-m32"], ["-m32"])
        AddBuild("build/release-32", allfeatures + allconfigs + releaseflags + cflags + ["-m32"], ["-m32"])
        AddBuild("build/debug-cxx-32", allfeatures + allconfigs + debugflags + cxxflags + ["-m32"], ["-m32", "-lstdc++"])
        AddBuild("build/release-cxx-32", allfeatures + allconfigs + releaseflags + cxxflags + ["-m32"], ["-m32", "-lstdc++"])

