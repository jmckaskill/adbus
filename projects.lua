local function directory(name)
  files {name .. "/**.c", name .. "/**.cpp", name .. "/**.h"}
end

local function common(name, type, id)
  project(name)
  uuid(id)
  kind(type)
  flags {"ExtraWarnings", "FatalWarnings"}
  directory(name)
  defines {"ADBUS_LITTLE_ENDIAN"}
  configuration {"windows"}
    defines {"LUA_BUILD_AS_DLL", "ADBUS_BUILD_AS_DLL"}
  configuration {"linux"}
    defines {"LUA_USE_LINUX"}
  configuration {}
end

local function ansi_project(name, type, id)
  common(name, type, id)
  language 'c'
  configuration {"vs*"}
    buildoptions {"/TC"} -- force it to compile as c
  configuration {}
end

local function c99_project(name, type, id)
  common(name, type, id)
  language 'c'
  configuration {"vs*"}
    buildoptions {"/TP"} -- force it to compile as c++
  configuration {"linux"}
    buildoptions {"--std=gnu99"}
  configuration {}
end

local function cpp_project(name, type, id)
  common(name, type, id)
  language 'c++'
end

c99_project('adbus', 'SharedLib', "6A120200-37D4-FA48-9838-B343A13D98C1")
  defines {"ADBUS_LIBRARY"}

cpp_project("adbuscpp", "StaticLib", "903F096F-1B59-1546-93BA-9295493C7AA4")
  defines {"ADBUSCPP_LIBRARY"}

c99_project('adbuslua', "SharedLib", "EE40C960-59A7-C849-92F2-14024DC8181A")
  targetname 'adbuslua_core'
  defines {"ADBUSLUA_LIBRARY"}
  links {"adbus", "lua5.1"}
  configuration {"windows"}
    links {"ws2_32", "advapi32", "user32"}

ansi_project("lua5.1", "SharedLib", "84C1D0B6-011F-B046-A5AA-EE30E0B8D214")
  defines {"LUA_LIBRARY"}
  directory 'lua-5.1.4/src'
  excludes {"lua-5.1.4/src/lua.c", "lua-5.1.4/src/luac.c"}
  configuration "linux"
    links {"dl", "m", "readline"}

ansi_project("lua", "ConsoleApp", "A1031290-BB35-CA4C-8F9E-F099DAA91ABA")
  files {"lua-5.1.4/src/lua.c"}
  links {"lua5.1", "adbuslua"}

c99_project("luaxml", "SharedLib", "9B52ED3C-AA46-5843-81F3-4FFA3C6EADED")
  targetname "LuaXML_lib"
  links "lua5.1"

c99_project("test-adbus", "ConsoleApp", "F05DC97B-044A-1847-AC92-8E39F3A7C10D")
  files {"adbus/vector.c",
         "adbus/str.c"}
  links {"adbus"}


