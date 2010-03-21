-- vim: ts=4 sw=4 sts=4 et

local function c_project(name)
    project(name)
    language 'c'
    includedirs {'deps/lua/src', 'deps', 'include/c'}
    defines {
        '__STDC_CONSTANT_MACROS',
        '__STDC_LIMIT_MACROS',
    }
    configuration 'linux'
        defines 'LUA_USE_LINUX'
        buildoptions {'-fPIC', '-std=gnu99'}
    configuration 'windows'
        defines 'LUA_BUILD_AS_DLL'
        links 'ws2_32'
    configuration 'vs*'
        defines '_CRT_SECURE_NO_DEPRECATE'
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

c_project 'adbus_ex_simple'
    uuid '9746F475-6975-F746-A956-A67A5878D9BA'
    kind 'ConsoleApp'
    files 'example/simple/*'
    links 'adbus'

c_project 'adbus_ex_ping_client'
    uuid 'BC0FDEB7-8D56-7C4D-93B1-8A9BA986E52D'
    kind 'ConsoleApp'
    files 'example/ping/client.c'
    links {'adbus', 'dmem'}

c_project 'adbus_ex_ping_client_async'
    uuid '8397BF65-386D-1842-A06F-4CEA6BE99B8F'
    kind 'ConsoleApp'
    files 'example/ping/client-async.c'
    links {'adbus', 'dmem'}

c_project 'adbus_ex_ping_server'
    uuid '3101CC1C-A88E-5B45-ADE4-3C6F9896A8B1'
    kind 'ConsoleApp'
    files 'example/ping/server.c'
    links 'adbus'

cpp_project 'adbus_ex_simplecpp'
    uuid '1CBDFB2E-BFCD-AA47-AE5D-0217C80C09F1'
    kind 'ConsoleApp'
    files 'example/simplecpp/*'
    links 'adbus'

cpp_project 'adbus_ex_bus_qt'
    uuid '60007A4D-DCEA-7448-ADDB-0E31984DE89D'
    kind 'ConsoleApp'
    files 'example/bus-qt/*'
    links {'adbus'}
    qt_libs {'Core', 'Network'}

cpp_project 'adbus_ex_client_qt'
    uuid '3fe0c954-c296-4a32-bd9d-341fbd1703ab' 
    kind 'ConsoleApp'
    files 'example/client-qt/*'
    links {'adbus'}
    qt_libs {'Core', 'Network'}

cpp_project 'adbus_ex_simple_qt'
    uuid '817b25a6-c404-4c3e-8150-b47c6b93bcce' 
    kind 'ConsoleApp'
    files 'example/simpleqt/*'
    links {'adbus', 'adbusqt'}
    qt_libs {'Core', 'Network'}
    includedirs {
        os.getenv('QTDIR') .. '/include/QtDBus',
        os.getenv('QTDIR') .. '/include/qt4/QtDBus',
    }

cpp_project 'adbus_ex_dbus_chat'
    uuid '4a827af0-2274-47f7-a61e-5fb5e8ed8fa4'
    kind 'WindowedApp'
    files 'example/qt-dbus/dbus-chat/*'
    links {'adbus', 'adbusqt'}
    qt_libs {'Core', 'Gui', 'Network'}
    includedirs {
        os.getenv('QTDIR') .. '/include/QtDBus',
        os.getenv('QTDIR') .. '/include/qt4/QtDBus',
    }


if os.is('windows') then
    cpp_project 'adbus_ex_bus_win'
        uuid 'E0DC1A67-7C2D-4DB5-82AD-7936B8CBB3A7'
        kind 'ConsoleApp'
        files 'example/bus-win/*'
        links {'adbus', 'dmem'}
end

if os.is('linux') then
    c_project 'adbus_ex_bus_epoll'
        kind 'ConsoleApp'
        files 'example/bus-epoll/*'
        links 'adbus'
end



