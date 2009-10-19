local function ansi_project(name, type, id)
  project(name)
  uuid(id)
  language 'c'
  kind(type)
  flags {"ExtraWarnings", "FatalWarnings"}
  files {name .. "/**.c", name .. "/**.h"}
  configuration {"vs*"}
    buildoptions {"/TC"} -- force it to compile as c
end

local function c99_project(name, type, id)
  project(name)
  uuid(id)
  language 'c'
  kind(type)
  flags {"ExtraWarnings", "FatalWarnings"}
  files {name .. "/**.c", name .. "/**.h"}
  configuration {"vs*"}
    buildoptions {"/TP"} -- force it to compile as c++
  configuration {"linux"}
    buildoptions {"--std=gnu99"}
end

local function cpp_project(name, type, id)
  project(name)
  uuid(id)
  language 'c++'
  kind(type)
  flags {"ExtraWarnings", "FatalWarnings"}
  files {name .. "/**.c", name .. "/**.h"}
end

local function lua_settings()
  defines {"LUA_LIBRARY"}
  configuration "linux"
    defines {"LUA_USE_LINUX"}
    links {"dl", "m", "readline"}
end

c99_project('adbus', 'SharedLib', "6A120200-37D4-FA48-9838-B343A13D98C1")
  defines {"ADBUS_LIBRARY", "ADBUS_LITTLE_ENDIAN"}


cpp_project("adbuscpp", "StaticLib", "903F096F-1B59-1546-93BA-9295493C7AA4")
  defines {"ADBUSCPP_LIBRARY"}

c99_project('adbuslua_core', "SharedLib", "EE40C960-59A7-C849-92F2-14024DC8181A")
  defines {"ADBUSLUA_LIBRARY"}
  links {"adbus", "lua"}
  configuration {"windows"}
    links {"ws2_32", "advapi32", "user32"}

ansi_project("lua", "SharedLib", "84C1D0B6-011F-B046-A5AA-EE30E0B8D214")
  targetname "lua5.1"
  files {"lua-5.1.4/src/**.h", "lua-5.1.4/src/**.c"}
  excludes {"lua-5.1.4/src/lua.c", "lua-5.1.4/src/luac.c"}
  lua_settings()

ansi_project("luaexec", "ConsoleApp", "A1031290-BB35-CA4C-8F9E-F099DAA91ABA")
  targetname "lua"
  files {"lua-5.1.4/src/lua.c"}
  links {"lua", "adbuslua_core"}
  lua_settings()

c99_project("luaxml", "SharedLib", "9B52ED3C-AA46-5843-81F3-4FFA3C6EADED")
  targetname "LuaXML_lib"
  links "lua"

c99_project("test-adbus", "ConsoleApp", "F05DC97B-044A-1847-AC92-8E39F3A7C10D")
  files {"adbus/vector.c",
         "adbus/str.c"}
  links {"adbus"}


