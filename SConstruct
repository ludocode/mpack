import platform, os

def CheckFlags(context, cppflags, linkflags = [], message = None):
    if message == None:
        message = " ".join(cppflags + ((cppflags != linkflags) and linkflags or []))
    context.Message("Checking for " + message + " support... ")

    env.Prepend(CPPFLAGS = cppflags, LINKFLAGS = linkflags)
    result = context.TryLink("int main(void){return 0;}\n", '.c')
    env.Replace(CPPFLAGS = env["CPPFLAGS"][len(cppflags):], LINKFLAGS = env["LINKFLAGS"][len(linkflags):])

    context.Result(result)
    return result


# Common environment setup

env = Environment()
conf = Configure(env, custom_tests = {'CheckFlags': CheckFlags})

for x in os.environ.keys():
    if x in ["CC", "CXX", "PATH", "TRAVIS", "TERM"] or x.startswith("CLANG_") or x.startswith("CCC_"):
        env[x] = os.environ[x]
        env["ENV"][x] = os.environ[x]

env.Append(CPPFLAGS = [
    "-Wall", "-Wextra", "-Werror",
    "-Wconversion", "-Wno-sign-conversion", "-Wundef",
    "-Isrc", "-Itest",
    "-DMPACK_SCONS=1",
    "-g",
    ])
env.Append(LINKFLAGS = [
    "-g",
    ])
# Additional warning flags are passed in SConscript based on the language (C/C++)

if 'TRAVIS' not in env:
    # Travis-CI currently uses Clang 3.4 which does not support this option,
    # and it also appears to be incompatible with other GCC options on Travis-CI
    env.Append(CPPFLAGS = ["-Wno-float-conversion"])


# Optional flags used in various builds

allfeatures = [
    "-DMPACK_READER=1",
    "-DMPACK_WRITER=1",
    "-DMPACK_EXPECT=1",
    "-DMPACK_NODE=1",
]
noioconfigs = [
    "-DMPACK_STDLIB=1",
    "-DMPACK_MALLOC=test_malloc",
    "-DMPACK_FREE=test_free",
]
allconfigs = noioconfigs + ["-DMPACK_STDIO=1"]

hasOg = conf.CheckFlags(["-Og"])
if hasOg:
    debugflags = ["-DDEBUG", "-Og"]
else:
    debugflags = ["-DDEBUG", "-O0"]
releaseflags = ["-Os"]
cflags = ["-std=c99"]

gcovflags = []
if ARGUMENTS.get('gcov'):
    gcovflags = ["-DMPACK_GCOV=1", "--coverage"]

ltoflags = ["-O3", "-flto", "-fuse-linker-plugin", "-fno-fat-lto-objects"]


# Functions to add a variant build. One variant build will build and run the
# entire library and test suite in a given configuration.

def AddBuild(variant_dir, cppflags, linkflags):
    env.SConscript("SConscript",
            variant_dir="build/" + variant_dir,
            src="../..",
            exports={
                'env': env,
                'CPPFLAGS': cppflags,
                'LINKFLAGS': linkflags
                },
            duplicate=0)

def AddBuilds(variant_dir, cppflags, linkflags):
    AddBuild("debug-" + variant_dir, debugflags + cppflags, debugflags + linkflags)
    AddBuild("release-" + variant_dir, releaseflags + cppflags, releaseflags + linkflags)


# The default build, everything in debug. This is the build used
# for code coverage measurement and static analysis.

AddBuild("debug", allfeatures + allconfigs + debugflags + cflags + gcovflags, gcovflags)


# Otherwise run all builds. Use a parallel build, e.g. "scons -j12" to
# build this in a reasonable amount of time.

if ARGUMENTS.get('all'):

    # release flags
    AddBuild("release", allfeatures + allconfigs + releaseflags + cflags, [])
    AddBuild("release-fastmath", allfeatures + allconfigs + releaseflags + cflags + ["-ffast-math"], [])
    AddBuild("release-speed", ["-DMPACK_OPTIMIZE_FOR_SIZE=0"] +
            allfeatures + allconfigs + releaseflags + cflags, [])
    if conf.CheckFlags(ltoflags, ltoflags, "-flto"):
        AddBuild("release-lto", allfeatures + allconfigs + ltoflags + cflags, ltoflags)
    if hasOg:
        AddBuild("debug-O0", allfeatures + allconfigs + ["-DDEBUG", "-O0"] + cflags, [])

    # feature subsets with default configuration
    AddBuilds("empty", allconfigs + cflags, [])
    AddBuilds("writer", ["-DMPACK_WRITER=1"] + allconfigs + cflags, [])
    AddBuilds("reader", ["-DMPACK_READER=1"] + allconfigs + cflags, [])
    AddBuilds("expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + allconfigs + cflags, [])
    AddBuilds("node", ["-DMPACK_NODE=1"] + allconfigs + cflags, [])

    # no i/o
    AddBuilds("noio", allfeatures + noioconfigs + cflags, [])
    AddBuilds("noio-writer", ["-DMPACK_WRITER=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-reader", ["-DMPACK_READER=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + noioconfigs + cflags, [])
    AddBuilds("noio-node", ["-DMPACK_NODE=1"] + noioconfigs + cflags, [])

    # embedded builds without libc
    AddBuilds("embed", allfeatures + cflags, [])
    AddBuilds("embed-writer", ["-DMPACK_WRITER=1"] + cflags, [])
    AddBuilds("embed-reader", ["-DMPACK_READER=1"] + cflags, [])
    AddBuilds("embed-expect", ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + cflags, [])
    AddBuilds("embed-node", ["-DMPACK_NODE=1"] + cflags, [])

    # miscellaneous test builds
    AddBuilds("notrack", ["-DMPACK_NO_TRACKING=1"] + allfeatures + allconfigs + cflags, [])
    AddBuilds("realloc", allfeatures + allconfigs + debugflags + cflags + ["-DMPACK_REALLOC=test_realloc"], [])
    if hasOg:
        AddBuild("debug-O0", allfeatures + allconfigs + ["-DDEBUG", "-O0"] + cflags, [])

    # other language standards (C11, various C++ versions)
    if conf.CheckFlags(["-std=c11"]):
        AddBuilds("c11", allfeatures + allconfigs + ["-std=c11"], [])
    AddBuilds("cxx", allfeatures + allconfigs + ["-xc++", "-std=c++98"], ["-lstdc++"])
    if conf.CheckFlags(["-xc++", "-std=c++11"], [], "-std=c++11"):
        AddBuilds("cxx11", allfeatures + allconfigs + ["-xc++", "-std=c++11"], ["-lstdc++"])
    if conf.CheckFlags(["-xc++", "-std=c++14"], [], "-std=c++14"):
        AddBuilds("cxx14", allfeatures + allconfigs + ["-xc++", "-std=c++14"], ["-lstdc++"])

    # 32-bit build
    if conf.CheckFlags(["-m32"], ["-m32"]):
        AddBuilds("32",     allfeatures + allconfigs + cflags + ["-m32"], ["-m32"])
        AddBuilds("cxx32",  allfeatures + allconfigs + ["-xc++", "-std=c++98", "-m32"], ["-lstdc++", "-m32"])
