@echo off

set LUA_PATH=%~dp0include\lua\?.lua;%~dp0include\lua\?\init.lua
set LUA_CPATH=%~dp0debug\?.dll

"%~dp0debug\lua.exe" %*

set LUA_PATH=
set LUA_CPATH=
