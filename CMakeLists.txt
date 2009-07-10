cmake_minimum_required(VERSION 2.6)

project(adbus)

if (WIN32)
  add_definitions(-DLUA_BUILD_AS_DLL
                  -DADBUS_BUILD_AS_DLL)
endif (WIN32)

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (MSVC)

add_subdirectory(adbus)
#add_subdirectory(adbuscpp)
add_subdirectory(adbuslua)
add_subdirectory(sha1-0.2)
add_subdirectory(lua-5.1.4)
add_subdirectory(luasocket-2.0.2)


