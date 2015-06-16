import platform

Import('env', 'CPPFLAGS', 'LINKFLAGS')

srcs = env.Object(env.Glob('src/mpack/*.c') + env.Glob('test/*.c'),
        CPPFLAGS=env['CPPFLAGS'] + CPPFLAGS)

prog = env.Program("mpack-test", srcs,
        LINKFLAGS=env['LINKFLAGS'] + LINKFLAGS)

if "TRAVIS" in env or (platform.machine() != "x86_64" and platform.machine() != "i386"):
    env.Default(env.AlwaysBuild(env.Alias("test",
        [prog],
        Dir('.').path + "/mpack-test")))
else:
    env.Default(env.AlwaysBuild(env.Alias("test",
        [prog],
        "valgrind --leak-check=full --errors-for-leak-kinds=all --show-leak-kinds=all " +
                "--suppressions=tools/valgrind-suppressions --error-exitcode=1 " +
                Dir('.').path + "/mpack-test")))

