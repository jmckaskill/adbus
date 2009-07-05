project(sha1)

include_directories(/usr/include/lua5.1)

add_library(sha1 SHARED
            sha1.cpp
            sha1.h
            sha1lua.cpp
            )

target_link_libraries(sha1 lua5.1)

install(TARGETS sha1 LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
