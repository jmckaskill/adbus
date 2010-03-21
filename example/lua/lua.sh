#!/bin/sh

export LD_LIBRARY_PATH="../../build"
export LUA_PATH="../../include/lua/?/init.lua;../../include/lua/?.lua;../../deps/LuaXML/?.lua"
export LUA_CPATH="../../build/lib?.so"

if [ $USE_GDB ]
then
    exec cgdb --args ../../build/lua "$@"
else
    exec ../../build/lua "$@"
fi
