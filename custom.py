CPPFLAGS = '`xml2-config --cflags` -Wall -pedantic-errors -I/usr/include/lua5.1'
LINKFLAGS = '`xml2-config --libs` `icu-config --ldflags`'
CPPDEFINES = ['LUA_EXTERN']
CPPPATH = ['/home/bjorn/workspace/icuwrap/src']
LIBPATH = ['/home/bjorn/workspace/icuwrap/build/debug']
