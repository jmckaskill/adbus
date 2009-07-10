project(sha1)

include_directories(../lua-5.1.4/src)

add_library(sha1 SHARED
            sha1.cpp
            sha1.h
            sha1lua.cpp
            )

target_link_libraries(sha1 lua)

set_target_properties(sha1 PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ../bin
                      LIBRARY_OUTPUT_DIRECTORY ../lib
                      ARCHIVE_OUTPUT_DIRECTORY ../lib)

