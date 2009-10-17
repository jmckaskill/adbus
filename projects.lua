project "adbus"
  uuid "6A120200-37D4-FA48-9838-B343A13D98C1"
  kind "SharedLib"
  language "c"
  flags {"ExtraWarnings", "FatalWarnings"}
  files {"adbus/**.c", "adbus/**.h"}
  defines {"ADBUS_LIBRARY", "ADBUS_LITTLE_ENDIAN"}
  configuration {"linux"}
    buildoptions {"--std=gnu99"}
  configuration {"windows"}
    buildoptions {"/TP"} -- compile as c++

project "adbuscpp"
  uuid "903F096F-1B59-1546-93BA-9295493C7AA4"
  kind "StaticLib"
  files {"adbuscpp/**.h", "adbuscpp/**.cpp"}
  language "c++"
  defines "ADBUSCPP_LIBRARY"
  flags {"ExtraWarnings", "FatalWarnings"}

project "adbuslua_core"
  uuid "EE40C960-59A7-C849-92F2-14024DC8181A"
  kind "SharedLib"
  language "c"
  files {"adbuslua/**.h", "adbuslua/**.lua", "adbuslua/**.c"}
  defines {"ADBUSLUA_LIBRARY"}
  links {"adbus", "lua"}
  flags {"ExtraWarnings", "FatalWarnings"}
  configuration {"linux"}
    buildoptions {"--std=gnu99"}
  configuration {"windows"}
    links {"ws2_32", "advapi32", "user32"}
  configuration {"vs*"}
    buildoptions {"/TP"} -- compile as c++

project "lua"
  uuid "84C1D0B6-011F-B046-A5AA-EE30E0B8D214"
  kind "SharedLib"
  language "c"
  targetname "lua5.1"
  files {"lua-5.1.4/src/**.h", "lua-5.1.4/src/**.c"}
  excludes {"lua-5.1.4/src/lua.c", "lua-5.1.4/src/luac.c"}
  defines "LUA_LIBRARY"
  configuration "vs*"
    buildoptions "/TC" -- compile as c
  configuration "linux"
    defines "LUA_USE_LINUX"
    links {"dl", "m", "readline"}

project "luaexec"
  uuid "A1031290-BB35-CA4C-8F9E-F099DAA91ABA"
  kind "ConsoleApp"
  language "c"
  targetname "lua"
  files {"lua-5.1.4/src/lua.c"}
  links {"lua"}
  configuration "vs*"
    buildoptions "/TC" -- compile as c
  configuration "linux"
    defines "LUA_USE_LINUX"
    links {"dl", "m", "readline"}

project "luaxml"
  uuid "9B52ED3C-AA46-5843-81F3-4FFA3C6EADED"
  kind "SharedLib"
  language "c"
  targetname "LuaXML_lib"
  files {"luaxml/*.c", "luaxml/*.h"}
  links "lua"
  configuration {"windows"}
    buildoptions {"/TP"} -- compile as c++

project "test-adbus"
  kind "ConsoleApp"
  language "c++"
  files {"test-adbus/**.c",
         "test-adbus/**.h",
         "adbus/vector.c",
         "adbus/str.c"}
  links {"adbus"}
  configuration {"windows"}
    buildoptions {"/TP"} -- compile as c++


