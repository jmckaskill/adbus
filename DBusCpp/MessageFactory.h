#pragma once

#include "Macro.h"

namespace DBus{

  //-----------------------------------------------------------------------------

  struct MessageRegistration
  {
  public:
    MessageRegistration():slot(NULL),type(InvalidMessageType),serial(0){}

    MessageType   type;
    std::string   service;
    std::string   path;
    std::string   interface;
    std::string   member;
    MethodBase*   slot;
    MethodBase*   errorSlot;
  };

  //-----------------------------------------------------------------------------

  class MessageFactory 
  {
  public:
    MessageFactory();
    ~MessageFactory();
    void reset();
    void setConnection(Connection* connection);
    void setService(const char* service, int size = -1);
    void setPath(const char* path, int size = -1);
    void setInterface(const char* interface, int size = -1);
    void setMember(const char* member, int size = -1);

    template<class U, class Fn>
    void setCallback(U* object, Fn function);

    template<class U, class Fn>
    void setErrorCallback(U* object, Fn function);



    template<DBUSCPP_DECLARE_TYPES_DEF>
    uint32_t call(DBUSCPP_CRARGS_DEF);

    template<DBUSCPP_DECLARE_TYPES_DEF>
    uint32_t callNoReply(DBUSCPP_CRARGS_DEF);

    uint32_t connectSignal();


  private:
    int setupMarshallerForCall(int flags = 0);

    std::string m_Service;
    std::string m_Path;
    std::string m_Interface;
    std::string m_Member;
    MessageRegistration m_Registration;
    DBusMarshaller* m_Marshaller;
    uint32_t m_Serial;
  };

  //-----------------------------------------------------------------------------

}
