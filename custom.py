CPPFLAGS = '`xml2-config --cflags` -Wall -pedantic-errors -I/usr/include/lua5.1'
LINKFLAGS = '`xml2-config --libs` `icu-config --ldflags`'
CPPDEFINES = ['LUA_EXTERN']
CPPPATH = ['../../../icuwrap/src']
LIBPATH = ['../../../icuwrap/build/debug']
