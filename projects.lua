-- vim: ts=4 sw=4 sts=4 et

local function c_project(name)
    project(name)
    language 'c'
    includedirs {'deps/lua/src', 'deps', 'include/c'}
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions {'-fPIC', '-std=gnu99'}
    configuration 'windows'
        defines 'LUA_BUILD_AS_DLL'
    configuration 'vs*'
        defines '_CRT_SECURE_NO_DEPRECATE'
        includedirs 'deps/msvc'
        buildoptions '/TP' -- force to compile as c++ since vc doesn't support iso
    configuration {}
end

c_project 'adbus'
    kind 'SharedLib'
    uuid '6A120200-37D4-FA48-9838-B343A13D98C1'
    files 'adbus/*'
    defines 'ADBUS_LIBRARY'
    links 'memory'
    configuration 'windows'
        links {'ws2_32', 'advapi32', 'user32'}

c_project 'adbuslua_core'
    kind 'SharedLib'
    uuid 'EE40C960-59A7-C849-92F2-14024DC8181A'
    files 'adbuslua/*'
    defines 'ADBUSLUA_LIBRARY'
    links 'adbus'
    configuration {'windows'}
        links {'lua5.1', 'ws2_32', 'advapi32', 'user32'}




