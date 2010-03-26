@echo off

set DBUS_SESSION_BUS_ADDRESS=
set LUA_PATH=%~dp0include\lua\?.lua;%~dp0include\lua\?\init.lua
set LUA_CPATH=%~dp0debug\?.dll
"%~dp0debug\lua.exe" -l adbus -e "print(adbus.connect_address())" > temp.txt
set LUA_PATH=
set LUA_CPATH=
set /p DBUS_SESSION_BUS_ADDRESS= < temp.txt
echo %DBUS_SESSION_BUS_ADDRESS%
del temp.txt
qdbus.exe %*

