@echo off

set LUA_PATH=!\..\include\lua\?.lua;!\..\include\lua\?\init.lua
set LUA_CPATH=!\?.dll

release\lua.exe %*

set LUA_PATH=
set LUA_CPATH=
