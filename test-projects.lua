-- vim: ts=4 sw=4 sts=4 et

local function c_project(name)
    project(name)
    language 'c'
    includedirs {'deps/lua/src', 'deps', 'include/c', '.'}
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions {'-fPIC', '-std=gnu99'}
    configuration 'windows'
        defines {
            'LUA_BUILD_AS_DLL',
            '__STDC_CONSTANT_MACROS',
            '__STDC_LIMIT_MACROS',
        }
    configuration 'vs*'
        defines '_CRT_SECURE_NO_DEPRECATE'
        includedirs 'deps/msvc'
        buildoptions '/TP' -- force to compile as c++ since vc doesn't support iso
    configuration {}
end

c_project 'adbus_test'
    uuid 'F05DC97B-044A-1847-AC92-8E39F3A7C10D'
    kind 'ConsoleApp'
    files 'test/*'
    links {'adbus', 'dmem'}




