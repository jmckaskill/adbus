project(luasocket)

include_directories(../lua-5.1.4/src)

add_definitions(-DLUASOCKET_EXPORTS
                -DLUASOCKET_API_DLLEXPORT
               )

add_library(core SHARED
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
            #src/unix.c
            #src/unix.h
            #src/usocket.c
            #src/usocket.h
            src/wsocket.c
            src/wsocket.h
            )

target_link_libraries(core ws2_32.lib lua)

                      
