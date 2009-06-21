project(DBusCpp)


include_directories(..)



add_library(DBusCpp SHARED 
            Arity.h
            Auth.h
            Auth.cpp
            Connection.cpp
            Connection.h
            Macros.h
            Marshall.h
            Marshall.cpp
            MessageFactory.cpp
            MessageFactory.h
            MessageFactory.inl
            Slot.h
            Slot_t.h
            sha1.h
            sha1.cpp
            )
target_link_libraries(DBusCpp DBusClient)

add_executable(DBusCppTest Test.cpp)
target_link_libraries(DBusCppTest DBusCpp DBusClient)

install(TARGETS DBusCppTest DBusCpp LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
