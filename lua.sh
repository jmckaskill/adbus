#!/bin/sh
export LUA_PATH="include/lua/?/init.lua;include/lua/?.lua;${LUA_PATH}"
#tup upd && exec cgdb --args ./lua "${@}"
#tup upd && exec valgrind ./lua "${@}"
#tup upd && exec ./lua "${@}"
exec ./lua "${@}"
