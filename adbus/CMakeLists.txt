project(adbus)

include(TestBigEndian)

test_big_endian(big_endian)
if (big_endian)
  add_definitions(-DADBUS_BIG_ENDIAN)
else (big_endian)
  add_definitions(-DADBUS_LITTLE_ENDIAN)
endif (big_endian)

add_definitions(-DADBUS_BUILDING)

if (MSVC)
  include_directories(../deps/msvc)
endif (MSVC)

set(code 
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

if (MSVC)
  set_source_files_properties(${code} PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library(adbus SHARED ${code} )


set_target_properties(adbus PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ../bin
                      LIBRARY_OUTPUT_DIRECTORY ../lib
                      ARCHIVE_OUTPUT_DIRECTORY ../lib)

