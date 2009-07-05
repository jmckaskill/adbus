project(adbuscpp)


include_directories(..)




#add_library(adbuscpp SHARED
#            Arity.h
#            Auth.h
#            Auth.cpp
#            Connection.cpp
#            Connection.h
#            Macros.h
#            Marshall.h
#            Marshall.cpp
#            MessageFactory.cpp
#            MessageFactory.h
#            MessageFactory.inl
#            Slot.h
#            Slot_t.h
#            sha1.h
#            sha1.cpp
#            )
#target_link_libraries(adbuscpp adbus)

add_executable(adbuscpp Test.cpp sha1.cpp Auth.cpp)
#target_link_libraries(adbuscpptest adbuscpp adbus)

install(TARGETS adbuscpp LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
