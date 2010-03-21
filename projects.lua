-- vim: ts=4 sw=4 sts=4 et

local function c_project(name)
    project(name)
    language 'c'
    includedirs {'deps/lua/src', 'deps', 'include/c', '.'}
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions {'-fPIC', '-std=gnu99'}
    configuration 'windows'
        defines 'LUA_BUILD_AS_DLL'
    configuration 'vs*'
        defines {
            '_CRT_SECURE_NO_DEPRECATE',
            '__STDC_CONSTANT_MACROS',
            '__STDC_LIMIT_MACROS',
        }
        includedirs 'deps/msvc'
        buildoptions '/TP' -- force to compile as c++ since vc doesn't support iso
    configuration {}
end

local function cpp_project(name)
    project(name)
    language 'c++'
    includedirs {'deps/lua/src', 'deps', 'include/c'}
    defines {
        '__STDC_CONSTANT_MACROS',
        '__STDC_LIMIT_MACROS',
    }
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions '-fPIC'
    configuration 'windows'
        defines 'LUA_BUILD_AS_DLL'
        links 'ws2_32'
    configuration 'vs*'
        defines '_CRT_SECURE_NO_DEPRECATE'
        includedirs 'deps/msvc'
    configuration {}
end

c_project 'dmem'
    language 'c++'
    kind 'StaticLib'
    uuid 'A8F0E5F1-636F-604B-86F6-B0A164DF1C17'
    files {'dmem/**.c', 'dmem/*.cpp', 'dmem/**.h', 'include/c/dmem/**.h'}


c_project 'adbus'
    kind 'SharedLib'
    uuid '6A120200-37D4-FA48-9838-B343A13D98C1'
    files 'adbus/*'
    files {'include/c/adbus*.h'}
    defines 'ADBUS_LIBRARY'
    links 'dmem'
    configuration 'windows'
        links {'ws2_32', 'advapi32', 'user32'}

c_project 'adbuslua_core'
    kind 'SharedLib'
    uuid 'EE40C960-59A7-C849-92F2-14024DC8181A'
    files 'adbuslua/*'
    files 'include/c/adbuslua.h'
    defines 'ADBUSLUA_LIBRARY'
    links {'adbus', 'dmem'}
    configuration {'windows'}
        links {'lua5.1', 'ws2_32', 'advapi32', 'user32'}

cpp_project 'adbusqt'
    kind 'SharedLib'
    uuid 'e00ca219-3ea5-46a7-9663-42d59a1b3c9b'
    files 'adbusqt/*'
    defines 'QDBUS_MAKEDLL'
    links {'adbus'}
    qt_libs {'Core', 'Network', 'Xml'}
    includedirs {
        os.getenv('QTDIR') .. '/include/QtDBus',
        os.getenv('QTDIR') .. '/include/qt4/QtDBus',
    }
    configuration {'windows', 'debug'}
        targetname 'QtDBusd4'
    configuration {'windows', 'release'}
        targetname 'QtDBus4'
    configuration {'not windows'}
        targetname 'QtDBus'




