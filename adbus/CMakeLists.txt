project(adbus)

include(TestBigEndian)

test_big_endian(big_endian)
if (big_endian)
  add_definitions(-DADBUS_BIG_ENDIAN)
else (big_endian)
  add_definitions(-DADBUS_LITTLE_ENDIAN)
endif (big_endian)

add_library(adbus SHARED
            Common.h
            Connection.h
            Connection_p.h
            Connection.cpp
            Marshaller.c
            Marshaller.h
            Message.c
            Message.h
            Message_p.h
            Misc.c
            Misc_p.h
            Parser.c
            Parser.h
            User.c
            User.h
            )

install(TARGETS adbus LIBRARY DESTINATION lib)
