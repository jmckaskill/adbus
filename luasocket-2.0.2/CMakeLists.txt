project(luasocket)

include_directories(../lua-5.1.4/src)

add_definitions(-DLUASOCKET_EXPORTS)
if (WIN32)
  add_definitions(-DLUASOCKET_API_DLLEXPORT)
endif (WIN32)

set(code
    src/auxiliar.c
    src/auxiliar.h
    src/buffer.c
    src/buffer.h
    src/except.c
    src/except.h
    src/inet.c
    src/inet.h
    src/io.c
    src/io.h
    src/luasocket.c
    src/luasocket.h
    src/mime.c
    src/mime.h
    src/options.c
    src/options.h
    src/select.c
    src/select.h
    src/socket.h
    src/tcp.c
    src/tcp.h
    src/timeout.c
    src/timeout.h
    src/udp.c
    src/udp.h
    )

if (WIN32)
  set(code ${code} 
      src/wsocket.c
      src/wsocket.h)
else (WIN32)
  set(code ${code}
      src/unix.c
      src/unix.h
      src/usocket.c
      src/usocket.h)
endif (WIN32)

add_library(luasocket_core SHARED ${code})
set_target_properties(luasocket_core PROPERTIES OUTPUT_NAME socket_core)

target_link_libraries(luasocket_core lua)
if (WIN32)
  target_link_libraries(luasocket_core ws2_32.lib)
endif (WIN32)

install(TARGETS luasocket_core
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES src/socket.lua
        DESTINATION bin)
                      
