Import('env', 'CPPFLAGS', 'LINKFLAGS')

srcs = env.Object(env.Glob('src/mpack/*.c') + env.Glob('test/*.c'),
        CPPFLAGS=env['CPPFLAGS'] + CPPFLAGS)

prog = env.Program("mpack-test", srcs,
        LINKFLAGS=env['LINKFLAGS'] + LINKFLAGS,
        LINK=(("-xc++" in CPPFLAGS) and 'g++' or 'gcc'))

env.Default(env.AlwaysBuild(env.Alias("test",
    [prog],
    "valgrind --leak-check=full " + Dir('.').path + "/mpack-test")))

