CPPFLAGS = '`xml2-config --cflags` -Wall -pedantic-errors -I/usr/include/lua5.1'
LINKFLAGS = '`xml2-config --libs`'
CPPDEFINES = ['LUA_EXTERN']
LIBPATH = ['/usr/local/lib']
