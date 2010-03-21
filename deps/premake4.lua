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


ansi_project 'lua5.1'
    uuid '84C1D0B6-011F-B046-A5AA-EE30E0B8D214'
    kind 'SharedLib'
    files {'lua/src/**.c', 'lua/src/**.h'}
    excludes {'lua/src/lua.c', 'lua/src/luac.c'}
    configuration 'linux'
        links {'dl', 'm', 'readline'}

ansi_project 'lua'
    uuid '98A6638F-8A91-3B4D-91FA-43E5FF619381'
    kind 'ConsoleApp'
    files 'lua/src/lua.c'
    links 'lua5.1'
    configuration 'linux'
        links {'dl', 'm', 'readline'}

c99_project 'LuaXML_lib'
    uuid '9B52ED3C-AA46-5843-81F3-4FFA3C6EADED'
    kind 'SharedLib'
    files 'LuaXML/*.c'
    includedirs 'lua/src'
    configuration 'windows'
        links 'lua5.1'

local function do_qt_libs(names, base, tail)
    libdirs(base .. '/lib')
    includedirs{base .. '/include', base .. '/include/qt4'}
    if type(names) == "table" then
        for _,lib in pairs(names) do
            includedirs{
                base .. '/include/Qt' .. lib,
                base .. '/include/qt4/Qt' .. lib,
            }
            links('Qt' .. lib .. tail)
        end
    else
        includedirs{
            base .. '/include/Qt' .. libs,
            base .. '/include/qt4/Qt' .. libs,
        }
        links('Qt' .. libs .. tail)
    end
end

function qt_libs(names)
    if not os.getenv("QTDIR") then
        error("Please set the QTDIR environment variable")
    end
    configuration {'linux'}
        do_qt_libs(names, os.getenv("QTDIR"), "")
    configuration {'windows', 'release'}
        do_qt_libs(names, os.getenv("QTDIR"), "4")
    configuration {'windows', 'debug'}
        do_qt_libs(names, os.getenv("QTDIR"), "d4")
    configuration {}
end
