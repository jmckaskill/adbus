project(luaxml)

include_directories(../lua-5.1.4/src)


set(code
    LuaXML_lib.c
    )

if (MSVC)
  set_source_files_properties(${code} PROPERTIES LANGUAGE CXX)
endif (MSVC)

add_library(LuaXML_lib SHARED ${code})

target_link_libraries(LuaXML_lib lua)

set_target_properties(LuaXML_lib PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ../bin
                      LIBRARY_OUTPUT_DIRECTORY ../lib
                      ARCHIVE_OUTPUT_DIRECTORY ../lib)
                      
