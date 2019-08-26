#!/usr/bin/env lua
--
-- Copyright (c) 2015-2019 Nicholas Fraser
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy of
-- this software and associated documentation files (the "Software"), to deal in
-- the Software without restriction, including without limitation the rights to
-- use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
-- the Software, and to permit persons to whom the Software is furnished to do so,
-- subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
-- FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
-- COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
-- IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
-- CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--

---------------------------------------------------

-- This is the buildsystem for the MPack unit test suite. It tests the compiler
-- for support for various flags and features, generates a ninja build file,
-- and runs ninja on it with the targets given on the command line.
--
-- This requires at least Lua 5.1, with rocks luafilesystem and rapidjson.

---------------------------------------------------
-- Dependencies
---------------------------------------------------

require 'lfs'
local rapidjson = require 'rapidjson'

-- Returns a new array containing the values of all given arrays in order
function concatArrays(...)
    local z = {}
    for _, t in ipairs({...}) do
        if type(t) ~= "table" then
            assert(false)
        end
        for _, value in ipairs(t) do
            table.insert(z, value)
        end
    end
    return z
end

-- Runs the given command, returning true if successful
function execute(args)
    local ret = os.execute(args)
    -- os.execute() return types changed between lua 5.1 and 5.3
    return type(ret) == "boolean" and ret or ret == 0
end

---------------------------------------------------
-- Setup
---------------------------------------------------

-- switch working directory to root of library
if debug.getinfo(1) ~= nil and debug.getinfo(1).short_src ~= nil then
    local debugname = debug.getinfo(1).short_src
    if debugname:find("/") then
		debugname = string.gsub(debugname, "(.*/)(.*)", "%1")
        lfs.chdir(debugname .. "/..")
    end
end

local ccvar = os.getenv("CC")
local config = {ccvar = os.getenv("CC")}

-- try to load cached config
local config_filename = "build/config/config.json"
local configfile = io.open(config_filename, "rb")
if configfile ~= nil then
    local configdata = configfile:read("*all")
    configfile:close()
    local loadedconfig = rapidjson.decode(configdata)
    if loadedconfig ~= nil and loadedconfig.ccvar == ccvar then
        print("Loaded cached configuration.")
        config = loadedconfig
    end
end

if config.flags == nil then
    config.flags = {}
end

---------------------------------------------------
-- Environment
---------------------------------------------------

if config.cc == nil then
    if ccvar ~= nil then
        config.cc = ccvar
    else
        -- prefer gcc. better warnings, faster code, slower compilation
        local ret = execute(table.concat({"which", "gcc", "1>/dev/null", "2>/dev/null"}, ' '))
        if ret then
            print("GCC found.")
            config.cc = "gcc"
        else
            print("GCC not found.")
            config.cc = "cc"
        end
    end
    print("Compiler: " .. config.cc)
end
local cc = config.cc

lfs.mkdir("build")
lfs.mkdir("build/config")

---------------------------------------------------
-- Test Flags
---------------------------------------------------

local out = io.open("build/config/flagtest.c", "wb")
out:write([[
int main(int argc, char** argv) {
    // array dereference to test for the existence of
    // sanitizer libs when using -fsanitize (libubsan)
    return argv[argc - 1] == 0;
}
]])
out:close()

-- Check whether the given flag is (or flags are) supported. If it's not in
-- cached config, it's tested and stored.
function checkFlag(flag)
    if config.flags[flag] ~= nil then
        -- io.write("Cached flag \"",flag,"\" ", config.flags[flag] and "supported" or "not supported.", "\n")
        return config.flags[flag]
    end
    io.write("Testing flag(s): " .. flag .. " ... ")
    io.flush()
    --io.write(table.concat({cc, flag, "build/config/flagtest.c", "-o", "build/config/flagtest", "1>&2"}, ' '))
    local ret = execute(table.concat({cc, "-Werror", flag, "build/config/flagtest.c", "-o", "build/config/flagtest", "2>/dev/null"}, ' '))
    if ret then
        print("Supported.")
        config.flags[flag] = true
    else
        print("Not supported.")
        config.flags[flag] = false
    end
    return config.flags[flag]
end

-- extra warnings. if we have them, we enable them in all builds.
extra_warnings_to_test = {
    "-Wpedantic",
    "-Wmissing-variable-declarations",
    "-Wfloat-conversion",
    "-fstrict-aliasing",
}

local extra_warnings = {}
config.extra_warnings = extra_warnings

for _, flag in ipairs(extra_warnings_to_test) do
    if checkFlag(flag) then
        table.insert(extra_warnings, flag)
    end
end

if checkFlag("-Wstrict-aliasing=3") then
    table.insert(extra_warnings, "-Wstrict-aliasing=3")
elseif checkFlag("-Wstrict-aliasing=2") then
    table.insert(extra_warnings, "-Wstrict-aliasing=2")
elseif checkFlag("-Wstrict-aliasing") then
    table.insert(extra_warnings, "-Wstrict-aliasing")
end

-- We use -Og for all debug builds if we have it, but ONLY under GCC. It can
-- sometimes improve warnings, and things run a lot faster especially under
-- Valgrind, but Clang stupidly maps it to -O1 which has some optimizations
-- that break debugging!
-- We don't bother checking if "cc" is actually "gcc" or any other
-- nonsense. We just lazily check if "gcc" is in the compiler name.
local hasOg = cc:find("gcc") and checkFlag("-Og")

isnanf_test = [[
#include <math.h>
int main(void) {
    return isnanf(5.5f);
}
]]

if config.isnanf == nil then
    io.write("Testing isnanf ... ")
    io.flush()
    local out = io.open("build/nantest.c", "wb")
    out:write(isnanf_test)
    out:close()
    local ret = execute(table.concat({cc, "build/nantest.c", "-o", "build/nantest", "2>/dev/null"}, ' '))
    if ret then
        print("Supported.")
        config.isnanf = true
    else
        print("Not supported.")
        config.isnanf = false
    end
end
if config.isnanf then
    table.insert(extra_warnings, '-DMPACK_ISNANF_IS_FUNC')
end


---------------------------------------------------
-- Other Flags
---------------------------------------------------

-- TODO: MPACK_HAS_CONFIG is off by default, so we should test without it. Unit
-- test configuration can be done with a force include file.
global_cppflags = concatArrays({
    "-Wall", "-Wextra", "-Werror",
    "-Wconversion", "-Wundef",
    "-Wshadow", "-Wcast-qual",
    "-Isrc", "-Itest",
    "-DMPACK_VARIANT_BUILDS=1",
    "-DMPACK_HAS_CONFIG=1",
    "-g",
}, extra_warnings)

if not os.getenv("CI") and checkFlag("-fdiagnostics-color=always") then
    global_cppflags = concatArrays(global_cppflags, {"-fdiagnostics-color=always"})
end

defaultfeatures = {
    "-DMPACK_READER=1",
    "-DMPACK_WRITER=1",
    "-DMPACK_EXPECT=1",
    "-DMPACK_NODE=1",
}

allfeatures = concatArrays(defaultfeatures, {
    "-DMPACK_COMPATIBILITY=1",
    "-DMPACK_EXTENSIONS=1",
})

noioconfigs = {
    "-DMPACK_STDLIB=1",
    "-DMPACK_MALLOC=test_malloc",
    "-DMPACK_FREE=test_free",
}

allconfigs = concatArrays({}, noioconfigs, {
    "-DMPACK_STDIO=1",
})

debugflags = {
    "-DDEBUG", hasOg and "-Og" or "-O0",
}

releaseflags = {
    "-O2",
}

cflags = {
    checkFlag("-std=c11") and "-std=c11" or "-std=c99",
    "-Wc++-compat"
}
if checkFlag("-Wmissing-prototypes") then
    cflags = concatArrays(cflags, {"-Wmissing-prototypes"})
end

cxxflags = {
    "-x", "c++",
    "-Wmissing-declarations",
}

---------------------------------------------------
-- Build configurations
---------------------------------------------------

builds = {}

function addBuild(name, cppflags, ldflags)
    builds[name] = {
        cppflags = cppflags,
        ldflags = ldflags
    }
end

function addDebugReleaseBuilds(name, cppflags, ldflags)
    addBuild(name .. "-debug", concatArrays(cppflags, debugflags), ldflags)
    addBuild(name .. "-release", concatArrays(cppflags, releaseflags), ldflags)
end

addDebugReleaseBuilds('default', concatArrays(defaultfeatures, allconfigs, cflags));
addDebugReleaseBuilds('everything', concatArrays(allfeatures, allconfigs, cflags));
builds["everything-debug"].run_wrapper = "valgrind"
builds["everything-release"].run_wrapper = "valgrind"
addDebugReleaseBuilds('empty', concatArrays(allconfigs, cflags));
addDebugReleaseBuilds('writer', concatArrays({"-DMPACK_WRITER=1"}, allconfigs, cflags));
addDebugReleaseBuilds('reader', concatArrays({"-DMPACK_READER=1"}, allconfigs, cflags));
addDebugReleaseBuilds('expect', concatArrays({"-DMPACK_READER=1", "-DMPACK_EXPECT=1"}, allconfigs, cflags));
addDebugReleaseBuilds('node', concatArrays({"-DMPACK_NODE=1"}, allconfigs, cflags));
addDebugReleaseBuilds('compatibility', concatArrays(defaultfeatures, {"-DMPACK_COMPATIBILITY=1"}, allconfigs, cflags));
addDebugReleaseBuilds('extensions', concatArrays(defaultfeatures, {"-DMPACK_EXTENSIONS=1"}, allconfigs, cflags));

-- no i/o
addDebugReleaseBuilds('noio', concatArrays(allfeatures, noioconfigs, cflags));
addDebugReleaseBuilds('noio-writer', concatArrays({"-DMPACK_WRITER=1"}, noioconfigs, cflags));
addDebugReleaseBuilds('noio-reader', concatArrays({"-DMPACK_READER=1"}, noioconfigs, cflags));
addDebugReleaseBuilds('noio-expect', concatArrays({"-DMPACK_READER=1", "-DMPACK_EXPECT=1"}, noioconfigs, cflags));
addDebugReleaseBuilds('noio-node', concatArrays({"-DMPACK_NODE=1"}, noioconfigs, cflags));

-- embedded builds without libc (using builtins)
addDebugReleaseBuilds('embed', concatArrays(allfeatures, cflags));
addDebugReleaseBuilds('embed-writer', concatArrays({"-DMPACK_WRITER=1"}, cflags));
addDebugReleaseBuilds('embed-reader', concatArrays({"-DMPACK_READER=1"}, cflags));
addDebugReleaseBuilds('embed-expect', concatArrays({"-DMPACK_READER=1", "-DMPACK_EXPECT=1"}, cflags));
addDebugReleaseBuilds('embed-node', concatArrays({"-DMPACK_NODE=1"}, cflags));
addDebugReleaseBuilds('embed-nobuiltins', concatArrays({"-DMPACK_NO_BUILTINS=1"}, allfeatures, cflags));

-- language versions
if checkFlag("-std=c11") then
    -- if we're using c11 for everything else, we still need to test c99
    addDebugReleaseBuilds('c99', concatArrays(allfeatures, allconfigs, {"-std=c99"}));
end
for _, version in ipairs({"c++11", "gnu++11", "c++14", "c++17"}) do
    local flags = concatArrays(cxxflags, {"-std=" .. version})
    if checkFlag(table.concat(flags, ' ')) then
        addDebugReleaseBuilds(version, concatArrays(allfeatures, allconfigs, flags));
    end
end

-- Make sure C++11 compiles with disabled features (see #66)
local cxx11flags = concatArrays(cxxflags, {"-std=c++11"})
if checkFlag(table.concat(cxx11flags, ' ')) then
    addDebugReleaseBuilds('c++11-empty', concatArrays(allconfigs, cxx11flags));
end

-- We disable pedantic in C++98 due to our use of variadic macros, trailing
-- commas, ll format specifiers, and probably more. We technically only support
-- C++98 with those extensions.
cxx98flags = concatArrays(cxxflags, {"-std=c++98"})
if checkFlag("-Wno-pedantic") then
    table.insert(cxx98flags, "-Wno-pedantic")
end
addDebugReleaseBuilds('c++98', concatArrays(allfeatures, allconfigs, cxx98flags))

-- 32-bit builds
if checkFlag("-m32") then
    addDebugReleaseBuilds('m32', concatArrays(allfeatures, allconfigs, cflags, {"-m32"}), {"-m32"});
    addDebugReleaseBuilds('cxx98-m32', concatArrays(allfeatures, allconfigs, cxx98flags, {"-m32"}), {"-m32"});
    if checkFlag(table.concat(cxx11flags, ' ')) then
        addDebugReleaseBuilds('c++11-m32', concatArrays(allfeatures, allconfigs, cxx11flags, {"-m32"}), {"-m32"});
    end
end

-- lto buld
local ltoflags = nil
if checkFlag("-O3 -flto -fuse-linker-plugin -fno-fat-lto-objects") then
    ltoflags = concatArrays(allfeatures, allconfigs, cflags,
        {"-O3", "-flto", "-fuse-linker-plugin", "-fno-fat-lto-objects"})
elseif checkFlag("-O3 -flto") then
    ltoflags = concatArrays(allfeatures, allconfigs, cflags, {"-O3", "-flto"})
end
if ltoflags then
    addBuild('lto', ltoflags, ltoflags)
    builds["lto"].run_wrapper = "valgrind"
end

-- miscellaneous special builds
addBuild('O3', concatArrays(allfeatures, allconfigs, cflags, {"-O3"}))
builds["O3"].run_wrapper = "valgrind"
addBuild('fastmath', concatArrays(allfeatures, allconfigs, cflags, {"-ffast-math"}))
builds["fastmath"].run_wrapper = "valgrind"
addDebugReleaseBuilds('optimize-size', concatArrays(allfeatures, allconfigs, cflags,
        {"-DMPACK_OPTIMIZE_FOR_SIZE=1 -DMPACK_STRINGS=0"}));
addBuild('coverage', concatArrays(allfeatures, allconfigs, cflags,
        {"-DMPACK_GCOV=1", "--coverage", "-fno-inline", "-fno-inline-small-functions", "-fno-default-inline"}),
        {"--coverage"})
addBuild('notrack', concatArrays(allfeatures, allconfigs, cflags, debugflags, {"-DMPACK_NO_TRACKING=1"}))
addDebugReleaseBuilds('realloc', concatArrays(allfeatures, allconfigs, cflags, {"-DMPACK_REALLOC=test_realloc"}))
builds["fastmath"].run_wrapper = "valgrind"
builds["coverage"].exclude = true -- don't run during "all". run separately by travis.
if hasOg then
    addBuild('O0', concatArrays(allfeatures, allconfigs, cflags, debugflags, {"-DDEBUG", "-O0"}))
end

-- sanitizers
function addSanitizerBuilds(name, cppflags, ldflags)
    if checkFlag(table.concat(cppflags, " ")) then
        addDebugReleaseBuilds(name, concatArrays(allfeatures, allconfigs, cflags, flags), ldflags)
    end
end
addSanitizerBuilds('sanitize-stack-protector', {"-Wstack-protector", "-fstack-protector-all"})
addSanitizerBuilds('sanitize-undefined', {"-fsanitize=undefined"})
addSanitizerBuilds('sanitize-safe-stack', {"-fsanitize=safe-stack"})
addSanitizerBuilds('sanitize-address', {"-fsanitize=address"}, {"-fsanitize=address"})
addSanitizerBuilds('sanitize-memory', {"-fsanitize=memory"})

sortedBuilds = {}
for build, _ in pairs(builds) do
    table.insert(sortedBuilds, build)
end
table.sort(sortedBuilds)

---------------------------------------------------
-- Ninja generation
---------------------------------------------------

srcs = {}

for _, dir in ipairs({"src/mpack", "test"}) do
    for file in lfs.dir(dir) do
        if #file > 2 and string.sub(file, -2) == ".c" then
            table.insert(srcs, dir .. "/" .. file)
        end
    end
end

local out = io.open("build/build.ninja", "wb")

out:write("# This file is auto-generated.\n")
out:write("# Do not edit it; your changes will be erased.\n")
out:write("\n")

-- 1.3 for gcc deps, 1.1 for pool
out:write("ninja_required_version = 1.3\n")
out:write("\n")

out:write("rule compile\n")
out:write(" command = ", cc, " ", checkFlag("-MMD") and "-MMD" or "-MD", " -MF $out.d $flags -c $in -o $out\n")
out:write(" deps = gcc\n")
out:write(" depfile = $out.d\n")
out:write("\n")

out:write("rule link\n")
out:write(" command = ", cc, " $flags $in -o $out\n")
out:write("\n")

-- unfortunately right now the unit tests all try to write to the same files,
-- so they break when run concurrently. we need to make it write to files under
-- that config's build/ folder; for now we just run them sequentially.
out:write("pool run_pool\n")
out:write(" depth = 1\n")
out:write("run_wrapper =\n")
out:write("rule run\n")
out:write(" command = $run_wrapper $in\n")
out:write(" pool = run_pool\n")
out:write("\n")

out:write("rule clean\n")
out:write(" command = rm -rf build .ninja_deps .ninja_log\n")
out:write("build clean: clean\n")
out:write("\n")

out:write("rule help\n")
out:write(" command = cat build/help\n")
out:write("build help: help\n")
out:write("\n")

for _, build in ipairs(sortedBuilds) do
    local flags = builds[build]
    local buildfolder = "build/" .. build
    local cppflags = concatArrays(global_cppflags, flags.cppflags)
    local ldflags = flags.ldflags or {}
    local objs = {}

    for _, src in ipairs(srcs) do
        local obj = buildfolder .. "/objs/" .. string.sub(src, 0, -3) .. ".o"
        table.insert(objs, obj)
        out:write("build ", obj, ": compile ", src, "\n")
        out:write(" flags = ", table.concat(cppflags, " "), "\n")
    end

    out:write("build ", buildfolder, "/runner: link ", table.concat(objs, " "), "\n")
    out:write(" flags = ", table.concat(ldflags, " "), "\n")

    -- You can omit "run-" in front of any build to just build it without
    -- running it. This lets you run it some other way (e.g. under gdb,
    -- with/without Valgrind, etc.)
    out:write("build ", build, ": phony ", buildfolder, "/runner\n\n")

    out:write("build run-", build, ": run ", buildfolder, "/runner\n")
    if builds[build].run_wrapper then
        out:write(" run_wrapper = ", builds[build].run_wrapper)
        if builds[build].run_wrapper == "valgrind" then
            out:write(" --leak-check=full --error-exitcode=1")
            out:write(" --suppressions=tools/valgrind-suppressions")
            out:write(" --show-leak-kinds=all --errors-for-leak-kinds=all")
        end
    end
    out:write("\n")
end

out:write("default run-everything-debug\n")
out:write("build default: phony run-everything-debug\n")
out:write("\n")

out:write("build more: phony run-everything-debug run-everything-release run-default-debug run-embed-debug run-embed-release")
if ltoflags then
    out:write(" run-lto")
end
out:write("\n\n")

out:write("build all: phony")
for _, build in ipairs(sortedBuilds) do
    if not builds[build].exclude then
        out:write(" run-")
        out:write(build)
    end
end
out:write("\n")

out:close()

local out = io.open("build/help", "wb")
out:write("\n")
out:write("Available targets:\n")
out:write("\n")
out:write("    (default)\n")
out:write("    more\n")
out:write("    all\n")
out:write("    clean\n")
out:write("    help\n")
out:write("\n")
for _, build in ipairs(sortedBuilds) do
    out:write("    run-" .. build .. "\n")
end
out:close()

---------------------------------------------------
-- Write cached config
---------------------------------------------------

-- done configuring. cache our config for later.
rapidjson.dump(config, config_filename, {pretty=true})

---------------------------------------------------
-- Ninja execution
---------------------------------------------------

local ninja = {"ninja", "-f", "build/build.ninja"}
for _, target in ipairs(arg) do
    table.insert(ninja, target)
end

local ret = execute(table.concat(ninja, ' '))
if not ret then
    os.exit(1)
end
