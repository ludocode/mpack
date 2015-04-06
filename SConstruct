import platform, os

env = Environment()
if os.environ.has_key('CC'):
    env['CC'] = os.environ['CC']

env.Append(CPPFLAGS = [
    "-Wall", "-Wextra", "-Werror",
    "-Isrc", "-Itest",
    "-g",
    ])
env.Append(LINKFLAGS = [
    "-g",
    "-fprofile-arcs", "-ftest-coverage"
    ])

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

AddBuild("build/debug",   debugflags + cflags, [])
if not ARGUMENTS.get('dev'):
    AddBuild("build/release", releaseflags + cflags, [])
    AddBuild("build/debug-cxx", debugflags + cxxflags, [])
    AddBuild("build/release-cxx", releaseflags + cxxflags, [])
    if '64' in platform.architecture()[0]:
        AddBuild("build/debug-32",   debugflags + cflags + ["-m32"], ["-m32"])
        AddBuild("build/release-32", releaseflags + cflags + ["-m32"], ["-m32"])
