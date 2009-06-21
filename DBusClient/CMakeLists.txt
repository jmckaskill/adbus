project(DBusClient)

include(TestBigEndian)

test_big_endian(big_endian)
if (big_endian)
  add_definitions(-DDBUSCLIENT_BIG_ENDIAN)
else (big_endian)
  add_definitions(-DDBUSCLIENT_LITTLE_ENDIAN)
endif (big_endian)

add_library(DBusClient SHARED
            Common.h
            Marshaller.c
            Marshaller.h
            Message.c
            Message.h
            Message_p.h
            Misc.c
            Misc_p.h
            Parser.c
            Parser.h
            )

install(TARGETS DBusClient LIBRARY DESTINATION lib)
