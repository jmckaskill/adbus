project(lua)

if (UNIX)
  add_definitions(-DLUA_USE_LINUX)
endif (UNIX)
  
set(code lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c
		lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c
		ltable.c ltm.c lundump.c lvm.c lzio.c
		lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c
		ltablib.c lstrlib.c loadlib.c linit.c)

add_library(lua SHARED ${code} )

if (UNIX)
  target_link_libraries(lua dl)
endif (UNIX)

install(TARGETS lua
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

