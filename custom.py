CPPFLAGS = '`xml2-config --cflags` `icu-config --cppflags` -I/usr/include/lua5.1 -Wfatal-errors -Wswitch-default -Wswitch-enum -Wunused-parameter -Wfloat-equal -Wundef -pedantic -Wall -Wextra'
LINKFLAGS = '`xml2-config --libs` `icu-config --ldflags`'
CPPDEFINES = ['LUA_EXTERN']
CPPPATH = ['../../../icuwrap/src']
LIBPATH = ['../../../icuwrap/build/debug']
