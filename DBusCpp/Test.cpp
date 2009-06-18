
#include <string>
#include <vector>
#include <cstdint>
#include "BlueCommon/Interface.h"
#include "BlueCommon/Connection.h"

class SomeRandomClass
{
public:

  SomeRandomClass();
  void registerInterfaces(const DBusRegistration& reg);

  void someFunc(int i);
  void someSecondFunc(const std::string& str);
  void thirdFunc(const std::vector<uint32_t>& values);
  void fourthFunc(const char* str);

  double someReturnValue(int i);

  void setSomeProp(double v);
  double someProp()const;

private:
  DBus::Signal<> m_Output;
};


void SomeRandomClass::registerInterfaces(DBus::Object* object)
{
  using namespace DBus;
  using boost::bind;

  ObjectInterface* i = object->addInterface("com.ctct.Random.Test1");

  i->addMethod("SomeFunc", &SomeRandomClass::someFunc, this)
   ->addArgument("some_param", "i")
   ->addAnnotation("com.ctct.Annotation", "Data");

  i->addMethod("SomeSecondFunc", &SomeRandomClass::someSecondFunc, this)
   ->addArgument("str", "s");

  i->addMethod("ThirdFunc", &SomeRandomClass::thirdFunc, this)
   ->addArgument("values", "au");

  i->addMethod("SomeReturnValue", &SomeRandomClass::someReturnValue, this)
   ->addReturn("return", "d")
   ->addArgument("argument", "d");

  i->addSignal("SomeOutput", &m_Output);

  i->addProperty<double>("SomeProp")
   ->setSetter(bind(&SomeRandomClass::setSomeProp, this, _1))
   ->setGetter(bind(&SomeRandomClass::someProp, this));



  ObjectInterface* other = object->addInterface("com.ctct.Other");
  
  other->addSignal("RandomSignal", &m_Output);
}
