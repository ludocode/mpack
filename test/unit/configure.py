#!/usr/bin/env python3

# Copyright (c) 2015-2020 Nicholas Fraser
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# This is the buildsystem configuration tool for the MPack unit test suite. It
# tests the compiler for support for various flags and features and generates a
# ninja build file to build the unit test suite in a variety of configurations.
#
# It can be run with a GCC-style compiler or with MSVC. To run it with MSVC,
# you must first have the Visual Studio build tools on your path. This means
# you need to either open a Visual Studio Build Tools command prompt, or source
# vsvarsall.bat for some version of the Visual Studio Build Tools.

import shutil, os, sys, subprocess
from os import path

globalbuild = path.join("test", ".build")
os.makedirs(globalbuild, exist_ok=True)



###################################################
# Determine Compiler
###################################################

cc = None
compiler = "unknown"

if os.getenv("CC"):
    cc = os.getenv("CC")
elif shutil.which("cl.exe"):
    cc = "cl"
else:
    cc = "cc"

if not shutil.which(cc):
    raise Exception("Compiler cannot be found!")

if cc.lower() == "cl" or cc.lower() == "cl.exe":
    compiler = "MSVC"
elif cc.endswith("cproc"):
    compiler = "cproc"
elif cc.endswith("chibicc"):
    compiler = "chibicc"
elif cc.endswith("8cc"):
    compiler = "8cc"
else:
    # try --version
    ret = subprocess.run([cc, "--version"], universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ret.returncode == 0:
        if ret.stdout.startswith("cparser "):
            compiler = "cparser"
        elif "clang" in ret.stdout:
            compiler = "Clang"

    if compiler == "unknown":
        # try -v
        ret = subprocess.run([cc, "-v"], universal_newlines=True,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if ret.returncode == 0:
            for line in (ret.stdout + "\n" + ret.stderr).splitlines():
                if line.startswith("tcc "):
                    compiler = "TinyCC"
                    break
                elif line.startswith("gcc "):
                    compiler = "GCC"
                    break

print("Using " + compiler + " compiler with executable: " + cc)

if compiler == "MSVC":
    obj_extension = ".obj"
    exe_extension = ".exe"
else:
    obj_extension = ".o"
    exe_extension = ""

msvc = (compiler == "MSVC")



###################################################
# Compiler Probing
###################################################

config = {
    "flags": {},
}

flagtest_src = path.join(globalbuild, "flagtest.c")
flagtest_exe = path.join(globalbuild, "flagtest" + exe_extension)
with open(flagtest_src, "w") as out:
    out.write("""
int main(int argc, char** argv) {
    // array dereference to test for the existence of
    // sanitizer libs when using -fsanitize (libubsan)
    // compare it to another string in the array so that
    // -Wzero-as-null-pointer-constant works
    return argv[argc - 1] == argv[0];
}
""")

def checkFlags(flags):
    if isinstance(flags, str):
        flags = [flags,]

    configArg = "|".join(flags)
    if configArg in config["flags"]:
        return config["flags"][configArg]
    print("Testing flag(s): " + " ".join(flags) + " ... ", end="")
    sys.stdout.flush()

    if msvc:
        cmd = [cc, "/WX"] + flags + [flagtest_src, "/Fe" + flagtest_exe]
    else:
        cmd = [cc, "-Werror"] + flags + [flagtest_src, "-o", flagtest_exe]
    ret = subprocess.run(cmd, universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    if ret.returncode == 0:
        print("Supported.")
        supported = True
    else:
        print("Not supported.")
        supported = False
    config["flags"][configArg] = supported
    return supported

def flagsIfSupported(flags):
    if checkFlags(flags):
        if isinstance(flags, str):
            return [flags]
        return flags
    return []

# We use -Og for all debug builds if we have it, but ONLY under GCC. It can
# sometimes improve warnings, and things run a lot faster especially under
# Valgrind, but Clang stupidly maps it to -O1 which has some optimizations
# that break debugging!
hasOg = False
print("Testing flag(s): -Og ... ", end="")
sys.stdout.flush()
if msvc:
    print("Not supported.")
else:
    ret = subprocess.run([cc, "-v"], universal_newlines=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    sys.stdout.flush()
    if ret.returncode != 0:
        print("Not supported.")
    else:
        for line in ret.stdout.splitlines():
            if line.startswith("gcc version"):
                hasOg = True
                break
        if hasOg:
            print("Supported.")
        else:
            print("May be supported, but we won't use it.")



###################################################
# Common Flags
###################################################

global_cppflags = []

if msvc:
    global_cppflags += [
        "/W3", "/WX",
    ]
else:
    global_cppflags += [
        "-Wall", "-Wextra", "-Werror",
        "-Wconversion", "-Wundef",
        "-Wshadow", "-Wcast-qual",
        "-g",
    ]

global_cppflags += [
    "-Isrc", "-Itest",
    "-DMPACK_VARIANT_BUILDS=1",
    "-DMPACK_HAS_CONFIG=1",
]

defaultfeatures = [
    "-DMPACK_READER=1",
    "-DMPACK_WRITER=1",
    "-DMPACK_EXPECT=1",
    "-DMPACK_NODE=1",
]

allfeatures = defaultfeatures + [
    "-DMPACK_COMPATIBILITY=1",
    "-DMPACK_EXTENSIONS=1",
]

noioconfigs = [
    "-DMPACK_STDLIB=1",
    "-DMPACK_MALLOC=test_malloc",
    "-DMPACK_FREE=test_free",
]

allconfigs = noioconfigs + [
    "-DMPACK_STDIO=1",
]

# optimization
if msvc:
    debugflags = ["/Od", "/MDd"]
    releaseflags = ["/O2", "/MD"]
else:
    debugflags = [hasOg and "-Og" or "-O0"]
    releaseflags = ["-O2"]
debugflags.append("-DDEBUG")
releaseflags.append("-DNDEBUG")

# flags for specifying source language
if msvc:
    cflags = ["/TC"]
    cxxflags = ["/TP"]
else:
    cflags = [
        checkFlags("-std=c11") and "-std=c11" or "-std=c99",
        "-Wc++-compat"
    ]
    cxxflags = [
        "-x", "c++",
        "-Wmissing-declarations",
    ]



###################################################
# Variable Flags
###################################################

if not os.getenv("CI"):
    # we have to force color diagnostics to get color output from ninja
    # (ninja will strip the colors if it's being piped)
    if checkFlags("-fdiagnostics-color=always"):
        global_cppflags.append("-fdiagnostics-color=always")
    elif checkFlags("-fcolor-diagnostics=always"):
        global_cppflags.append("-color-diagnostics=always")

if checkFlags("-Wstrict-aliasing=3"):
    global_cppflags.append("-Wstrict-aliasing=3")
elif checkFlags("-Wstrict-aliasing=2"):
    global_cppflags.append("-Wstrict-aliasing=2")
elif checkFlags("-Wstrict-aliasing"):
    global_cppflags.append("-Wstrict-aliasing")

extra_warnings_to_test = [
    "-Wpedantic",
    "-Wmissing-variable-declarations",
    "-Wfloat-conversion",
    "-fstrict-aliasing",
]
for flag in extra_warnings_to_test:
    global_cppflags += flagsIfSupported(flag)

cflags += flagsIfSupported("-Wmissing-prototypes")

#TODO
ltoflags = []



###################################################
# Build configuration
###################################################

builds = {}

class Build:
    def __init__(self, name, cppflags, ldflags):
        self.name = name
        self.cppflags = cppflags
        self.ldflags = ldflags
        self.run_wrapper = None
        self.exclude = False

def addBuild(name, cppflags, ldflags=[]):
    builds[name] = Build(name, cppflags, ldflags)

def addDebugReleaseBuilds(name, cppflags, ldflags = []):
    addBuild(name + "-debug", cppflags + debugflags, ldflags)
    addBuild(name + "-release", cppflags + releaseflags, ldflags)

addDebugReleaseBuilds('default', defaultfeatures + allconfigs + cflags)
addDebugReleaseBuilds('everything', allfeatures + allconfigs + cflags)
addDebugReleaseBuilds('empty', allconfigs + cflags)
addDebugReleaseBuilds('reader', ["-DMPACK_READER=1"] + allconfigs + cflags)
addDebugReleaseBuilds('expect', ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + allconfigs + cflags)
addDebugReleaseBuilds('node', ["-DMPACK_NODE=1"] + allconfigs + cflags)
addDebugReleaseBuilds('compatibility', defaultfeatures + ["-DMPACK_COMPATIBILITY=1"] + allconfigs + cflags)
addDebugReleaseBuilds('extensions', defaultfeatures + ["-DMPACK_EXTENSIONS=1"] + allconfigs + cflags)

# writer builds
addDebugReleaseBuilds('writer-only',
        ["-DMPACK_WRITER=1", "-DMPACK_BUILDER=0"]
        + allconfigs + cflags)
addDebugReleaseBuilds('builder-internal',
        ["-DMPACK_WRITER=1", "-DMPACK_BUILDER=1", "-DMPACK_BUILDER_INTERNAL_STORAGE=1"]
        + allconfigs + cflags)
addDebugReleaseBuilds('builder-nointernal',
        ["-DMPACK_WRITER=1", "-DMPACK_BUILDER=1", "-DMPACK_BUILDER_INTERNAL_STORAGE=0"]
        + allconfigs + cflags)

# no i/o
addDebugReleaseBuilds('noio', allfeatures + noioconfigs + cflags)
addDebugReleaseBuilds('noio-writer', ["-DMPACK_WRITER=1"] + noioconfigs + cflags)
addDebugReleaseBuilds('noio-reader', ["-DMPACK_READER=1"] + noioconfigs + cflags)
addDebugReleaseBuilds('noio-expect', ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + noioconfigs + cflags)
addDebugReleaseBuilds('noio-node', ["-DMPACK_NODE=1"] + noioconfigs + cflags)

# embedded builds without libc (using builtins)
addDebugReleaseBuilds('embed', allfeatures + cflags)
addDebugReleaseBuilds('embed-writer', ["-DMPACK_WRITER=1"] + cflags)
addDebugReleaseBuilds('embed-reader', ["-DMPACK_READER=1"] + cflags)
addDebugReleaseBuilds('embed-expect', ["-DMPACK_READER=1", "-DMPACK_EXPECT=1"] + cflags)
addDebugReleaseBuilds('embed-node', ["-DMPACK_NODE=1"] + cflags)
addDebugReleaseBuilds('embed-nobuiltins', ["-DMPACK_NO_BUILTINS=1"] + allfeatures + cflags)

haveValgrind = shutil.which("valgrind")

if haveValgrind:
    # use valgrind for everything build if available
    builds["everything-debug"].run_wrapper = "valgrind"
    builds["everything-release"].run_wrapper = "valgrind"

# language versions
if msvc:
    addDebugReleaseBuilds('c++', allfeatures + allconfigs + cxxflags)
elif compiler != "TinyCC":
    if checkFlags("-std=c11"):
        # if we're using c11 for everything else, we still need to test c99
        addDebugReleaseBuilds('c99', allfeatures + allconfigs + ["-std=c99"])
    for version in ["c++11", "gnu++11", "c++14", "c++17"]:
        flags = cxxflags + ["-std=" + version]
        if checkFlags(flags):
            addDebugReleaseBuilds(version, allfeatures + allconfigs + flags)

    # Make sure C++11 compiles with disabled features (see #66)
    cxx11flags = cxxflags + ["-std=c++11"]
    if checkFlags(cxx11flags):
        addDebugReleaseBuilds('c++11-empty', allconfigs + cxx11flags)

    # We disable pedantic in C++98 due to our use of variadic macros, trailing
    # commas, ll format specifiers, and probably more. We technically only support
    # C++98 with those extensions.
    cxx98flags = cxxflags + ["-std=c++98"]
    if checkFlags("-Wno-pedantic"):
        cxx98flags += ["-Wno-pedantic"]
    addDebugReleaseBuilds('c++98', allfeatures + allconfigs + cxx98flags)

# 32-bit builds
if not msvc and checkFlags("-m32"):
    addDebugReleaseBuilds('m32', allfeatures + allconfigs + cflags + ["-m32"], ["-m32"])
    addDebugReleaseBuilds('cxx98-m32', allfeatures + allconfigs + cxx98flags + ["-m32"], ["-m32"])
    if checkFlags(cxx11flags):
        addDebugReleaseBuilds('c++11-m32', allfeatures + allconfigs + cxx11flags + ["-m32"], ["-m32"])

# lto build
if msvc:
    addBuild('lto', allfeatures + allconfigs + cflags + releaseflags + ["/GL"], ["/LTCG"])
elif compiler != "TinyCC":
    ltoflags = ["-O3", "-flto", "-fuse-linker-plugin", "-fno-fat-lto-objects"]
    if checkFlags(ltoflags):
        ltoflags = allfeatures + allconfigs + cflags + ltoflags
    else:
        ltoflags = ["-O3", "-flto"]
        if checkFlags(ltoflags):
            ltoflags = allfeatures + allconfigs + cflags + ltoflags
        else:
            ltoflags = None
    if ltoflags:
        addBuild('lto', ltoflags, ltoflags)
        if haveValgrind:
            builds["lto"].run_wrapper = "valgrind"

# size-optimized build (both debug and release)
if msvc:
    sizeOptimize = ["/O1", "/MD"]
else:
    sizeOptimize = ["-Os"]
addBuild('optimize-size-debug', allfeatures + allconfigs + cflags + sizeOptimize +
        ["-DMPACK_OPTIMIZE_FOR_SIZE=1", "-DMPACK_STRINGS=0", "-DDEBUG"])
addBuild('optimize-size-release', allfeatures + allconfigs + cflags + sizeOptimize +
        ["-DMPACK_OPTIMIZE_FOR_SIZE=1", "-DMPACK_STRINGS=0", "-DNDEBUG"])

# miscellaneous special builds
addBuild('notrack', allfeatures + allconfigs + cflags + debugflags + ["-DMPACK_NO_TRACKING=1"])
addDebugReleaseBuilds('realloc', allfeatures + allconfigs + cflags + ["-DMPACK_REALLOC=test_realloc"])
if not msvc and compiler != "TinyCC":
    addBuild('O3', allfeatures + allconfigs + cflags + ["-O3"])
    if haveValgrind:
        builds["O3"].run_wrapper = "valgrind"
    addBuild('fastmath', allfeatures + allconfigs + cflags + ["-ffast-math"])
    if haveValgrind:
        builds["fastmath"].run_wrapper = "valgrind"
    addBuild('coverage', allfeatures + allconfigs + cflags +
            ["-DMPACK_GCOV=1", "--coverage", "-fno-inline", "-fno-inline-small-functions", "-fno-default-inline"],
            ["--coverage"])
    builds["coverage"].exclude = True # don't run coverage during "all". run separately by CI.
    if hasOg:
        addBuild('O0', allfeatures + allconfigs + cflags + ["-DDEBUG", "-O0"])

# sanitizers
if msvc:
    # https://devblogs.microsoft.com/cppblog/asan-for-windows-x64-and-debug-build-support/
    addDebugReleaseBuilds('sanitize-address', allfeatures + allconfigs + cflags + ["/fsanitize=address"],
            ["/wholearchive:clang_rt.asan_dynamic-x86_64.lib",
            "/wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib"])
elif compiler != "TinyCC":
    def addSanitizerBuilds(name, cppflags, ldflags=[]):
        if checkFlags(cppflags):
            addDebugReleaseBuilds(name, allfeatures + allconfigs + cflags + cppflags, ldflags)
    addSanitizerBuilds('sanitize-undefined', ["-fsanitize=undefined"], ["-fsanitize=undefined"])
    addSanitizerBuilds('sanitize-safe-stack', ["-fsanitize=safe-stack"], ["-fsanitize=safe-stack"])
    addSanitizerBuilds('sanitize-address', ["-fsanitize=address"], ["-fsanitize=address"])
    addSanitizerBuilds('sanitize-memory', ["-fsanitize=memory"], ["-fsanitize=memory"])
    # not technically a sanitizer, but close enough:
    addSanitizerBuilds('sanitize-stack-protector', ["-Wstack-protector", "-fstack-protector-all"])



###################################################
# Ninja generation
###################################################

srcs = []

for paths in [path.join("src", "mpack"), "test"]:
    for root, dirs, files in os.walk(paths):
        if ".build" in root:
            continue
        for name in files:
            if name == "fuzz.c":
                continue
            if name.endswith(".c"):
                srcs.append(os.path.join(root, name))

ninja = path.join(globalbuild, "build.ninja")
with open(ninja, "w") as out:
    out.write("# This file is auto-generated.\n")
    out.write("# Do not edit it; your changes will be erased.\n")
    out.write("\n")

    # 1.3 for gcc deps, 1.1 for pool
    out.write("ninja_required_version = 1.3\n")
    out.write("\n")

    out.write("rule compile\n")
    if msvc:
        out.write(" command = " + cc + " /showIncludes $flags /c $in /Fo$out\n")
        out.write(" deps = msvc\n")
    else:
        out.write(" command = " + cc + " " + "-MD -MF $out.d $flags -c $in -o $out\n")
        out.write(" deps = gcc\n")
        out.write(" depfile = $out.d\n")
    out.write("\n")

    out.write("rule link\n")
    if msvc:
        out.write(" command = link $flags $in /OUT:$out\n")
    else:
        out.write(" command = " + cc + " $flags $in -o $out\n")
    out.write("\n")

    # unfortunately right now the unit tests all try to write to the same files,
    # so they break when run concurrently. we need to make it write to files under
    # that config's build/ folder; for now we just run them sequentially.
    out.write("pool run_pool\n")
    out.write(" depth = 1\n")
    out.write("run_wrapper =\n")
    out.write("rule run\n")
    out.write(" command = $run_wrapper$in\n")
    out.write(" pool = run_pool\n")
    out.write("\n")

    out.write("rule help\n")
    out.write(" command = cat build/help\n")
    out.write("build help: help\n")
    out.write("\n")

    for buildname in sorted(builds.keys()):
        build = builds[buildname]
        buildfolder = path.join(globalbuild, buildname)
        cppflags = global_cppflags + build.cppflags
        ldflags = build.ldflags
        objs = []

        for src in srcs:
            obj = path.join(buildfolder, "objs", src[:-2] + obj_extension)
            objs.append(obj)
            out.write("build " + obj + ": compile " + src + "\n")
            out.write(" flags = " + " ".join(cppflags) + "\n")

        runner = path.join(buildfolder, "runner") + exe_extension

        out.write("build " + runner + ": link " + " ".join(objs) + "\n")
        out.write(" flags = " + " ".join(ldflags) + "\n")

        # You can omit "run-" in front of any build to just build it without
        # running it. This lets you run it some other way (e.g. under gdb,
        # with/without Valgrind, etc.)
        out.write("build " + buildname + ": phony " + runner + "\n\n")

        out.write("build run-" + buildname + ": run " + runner + "\n")
        if build.run_wrapper:
            run_wrapper = build.run_wrapper
            out.write(" run_wrapper = " + run_wrapper + " ")
            if run_wrapper == "valgrind":
                out.write("--leak-check=full --error-exitcode=1 ")
                out.write("--suppressions=tools/valgrind-suppressions ")
                out.write("--show-leak-kinds=all --errors-for-leak-kinds=all ")
        out.write("\n")

    out.write("default run-everything-debug\n")
    out.write("build default: phony run-everything-debug\n")
    out.write("\n")

    out.write("build more: phony run-everything-debug run-everything-release run-default-debug run-embed-debug run-embed-release")
    if ltoflags:
        out.write(" run-lto")
    out.write("\n\n")

    out.write("build all: phony")
    for build in sorted(builds.keys()):
        if not builds[build].exclude:
            out.write(" run-")
            out.write(build)
    out.write("\n")

print("Generated " + ninja)

with open(path.join(globalbuild, "help"), "w") as out:
    out.write("\n")
    out.write("Available targets:\n")
    out.write("\n")
    out.write("    (default)\n")
    out.write("    more\n")
    out.write("    all\n")
    out.write("    clean\n")
    out.write("    help\n")
    out.write("\n")
    for build in sorted(builds.keys()):
        out.write("    run-" + build + "\n")
    out.close()
