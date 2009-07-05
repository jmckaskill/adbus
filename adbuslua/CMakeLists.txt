project(adbuslua)

include_directories(.. /usr/include/lua5.1)

add_library(adbuslua_core SHARED
            ADBusLua.c
            ADBusLua.h
            Connection.h
            Data.h
            Interface.h
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

target_link_libraries(adbuslua_core adbus lua5.1)

add_executable(adbuslua_test lua.c)
target_link_libraries(adbuslua_test adbuslua_core)

install(TARGETS adbuslua_core adbuslua_test LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
