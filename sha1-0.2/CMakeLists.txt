project(sha1)

include_directories(../lua-5.1.4/src)

add_library(sha1 SHARED
            sha1.cpp
            sha1.h
            sha1lua.cpp
            )

target_link_libraries(sha1 lua)

install(TARGETS sha1
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

