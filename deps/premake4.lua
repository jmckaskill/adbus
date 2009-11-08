-- vim: ts=4 sw=4 sts=4 et

local function project_common(name)
    project(name)
    language 'c'
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions '-fPIC'
    configuration 'windows'
        defines 'LUA_BUILD_AS_DLL'
    configuration 'vs*'
        includedirs 'deps/msvc'
        defines '_CRT_SECURE_NO_DEPRECATE'
    configuration {}
end


local function c99_project(name)
    project_common(name)
    configuration 'linux'
        buildoptions '-std=gnu99'
    configuration 'vs*'
        buildoptions '/TP' -- force to compile as c++ since vc doesn't support iso
    configuration {}
end

local function ansi_project(name)
    project_common(name)
    configuration 'vs*'
        buildoptions '/TC' -- force to compile as c
    configuration {}
end


c99_project 'memory'
    kind 'StaticLib'
    uuid 'A8F0E5F1-636F-604B-86F6-B0A164DF1C17'
    files {'memory/**.c', 'memory/**.h'}

ansi_project 'lua5.1'
    kind 'SharedLib'
    uuid '84C1D0B6-011F-B046-A5AA-EE30E0B8D214'
    files {'lua/src/**.c', 'lua/src/**.h'}
    excludes {'lua/src/lua.c', 'lua/src/luac.c'}
    configuration 'linux'
        links {'dl', 'm', 'readline'}

ansi_project 'lua'
    kind 'ConsoleApp'
    files 'lua/src/lua.c'
    links 'lua5.1'

c99_project 'LuaXML_lib'
    kind 'SharedLib'
    uuid '9B52ED3C-AA46-5843-81F3-4FFA3C6EADED'
    files 'LuaXML/*.c'
    includedirs 'lua/src'
    configuration 'windows'
        links 'lua5.1'
