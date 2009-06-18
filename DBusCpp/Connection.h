#pragma once


namespace DBus{

  //-----------------------------------------------------------------------------

  class Error
  {
  public:
    virtual const char* errorName()const=0;
    virtual const char* errorMessage()const=0;
  };

  //-----------------------------------------------------------------------------

  class Connection 
  {
    NON_COPYABLE(Connection);
  public:
    Connection();
    ~Connection();

    void setSendCallback(DBusSendCallback callback, void* callbackData);
    int  appendInputData(uint8_t* data, size_t size);

    void connectToBus();

    void addService(const char* name, int size = -1);
    void removeService(const char* name, int size = -1);
    const std::string& uniqueName()const{return m_UniqueName;}
    bool isConnected()const{return m_Connected;}

    void setupMarshaller(DBusMarshaller* marshaller, uint32_t* serial);
    
    uint32_t addRegistration(const MessageRegistration& registration);

    Object* addObject(const char* name, int size = -1);
    void removeObject(const char* name, int size = -1);

  private:
    friend class RegistrationHandle;
    typedef std::vector<MessageRegistration> MessageRegistrations;

    static void parserCallback(void* connection, Message* message);
    void dispatchToRegistration(Message* message);
    void dispatchToObject(Message* message);
    void onHello(const char* uniqueName);

    DBusSendCallback      m_Callback;
    void*                 m_CallbackData;
    Parser*               m_Parser;
    std::vector<uint8_t>  m_InputBuffer;
    MessageRegistrations  m_Registrations;
    MessageFactory        m_BusFactory;
    std::set<Object*>     m_Objects;
    DBusMarshaller*       m_ErrorMarshaller;
    uint32_t              m_NextSerial;
    bool                  m_Connected;
    std::string           m_UniqueName;
  };

  //-----------------------------------------------------------------------------

  class Object
  {
    NON_COPYABLE(Object);
  public:
    ObjectInterface* addInterface(const char* name);
    void removeInterface(const char* name);

    void introspect(std::string& out)const;

    const std::string& name()const{return m_Name;}

    Connection* connection()const{return m_Connection;}

  private:
    friend class Connection;
    friend class ObjectInterface;

    Object(Connection* connection);
    ~Object();

    void callMethod(Message* message);
    void setName(const char* name, int size = -1);

    DBusMarshaller* marshaller();

    typedef std::set<ObjectInterface*> Interfaces;

    std::string m_Name;
    Interfaces m_Interfaces;
    Connection* m_Connection;
    DBusMarshaller* m_Marshaller;
  };

  struct StringRef
  {
    StringRef(const char* str_, int size_)
      :str(str_),size(size_){}

    const char* str;
    int size;
  };

  //-----------------------------------------------------------------------------

}

