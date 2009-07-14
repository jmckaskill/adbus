project(adbuslua)

include_directories(.. ../lua-5.1.4/src)

if (WIN32)
  add_definitions(-DWIN32)
endif (WIN32)

if (MSVC)
  include_directories(../deps/msvc)
endif (MSVC)

set(code
    ADBusLua.c
    ADBusLua.h
    Connection.h
    Data.h
    Interface.h
    LuaInclude.h
    Match.h
    Message.h
    Object.h
    Connection.c
    Data.c
    Interface.c
    Match.c
    Message.c
    Object.c
    )

if (MSVC)
  set_source_files_properties(${code} PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library(adbuslua_core SHARED ${code})
set_target_properties(adbuslua_core PROPERTIES DEFINE_SYMBOL LADBUS_BUILDING)

target_link_libraries(adbuslua_core adbus lua)
if (WIN32)
  target_link_libraries(adbuslua_core advapi32.lib)
endif (WIN32)

add_executable(adbuslua_test lua.c)
target_link_libraries(adbuslua_test adbuslua_core)


install(TARGETS adbuslua_test adbuslua_core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES init.lua Connect.lua Connection.lua Proxy.lua
        DESTINATION bin/adbuslua)

install(FILES Test.lua DESTINATION bin)

