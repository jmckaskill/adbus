// vim: ts=2 sw=2 sts=2 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#include "Auth.h"
#include "Connection.h"
#include "Connection.inl"

#include "sha1.h"

#include <boost/bind.hpp>

#include <string>
#include <vector>

#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class SomeRandomClass
{
public:

  SomeRandomClass(){}
  void registerInterfaces(adbus::Object* object);

  void someFunc(int i)
  {
    i;
  }
  void someSecondFunc(const std::string& str)
  {
    str;
  }
  void thirdFunc(const std::vector<uint32_t>& values)
  {
    values;
  }
  void fourthFunc(const char* str)
  {
    str;
  }

  double someReturnValue(int i)
  {
    return i;
  }

  void setSomeProp(double v)
  {
    v;
  }
  double someProp()const
  {
    return 20.1;
  }

private:
  adbus::Signal<> m_Output;
};
ADBUSCPP_DECLARE_TYPE_STRING(std::vector<uint32_t>,  "au")
ADBUSCPP_DECLARE_BASE_TYPE(const std::vector<uint32_t>&, std::vector<uint32_t>)


void SomeRandomClass::registerInterfaces(adbus::Object* object)
{
  using namespace adbus;

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
   ->addArgument("argument", "i");

  i->addSignal("SomeOutput", &m_Output);

#if 0
  i->addProperty<double>("SomeProp")
   ->setSetter(boost::bind(&SomeRandomClass::setSomeProp, this, _1))
   ->setGetter(boost::bind(&SomeRandomClass::someProp, this));
#endif



  ObjectInterface* other = object->addInterface("com.ctct.Other");

  other->addSignal("RandomSignal", &m_Output);
}

static int tcpConnect(const char* address, const char* service)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s;

  /* Obtain address(es) matching host/port */

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */

  s = getaddrinfo(address, service, &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
        rp->ai_protocol);
    if (sfd == -1)
      continue;

    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;                  /* Success */

    close(sfd);
  }

  if (rp == NULL) {               /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);           /* No longer needed */

  return sfd;
}

static void SendData(void* sock, uint8_t* data, size_t len)
{
  int fd = *(int*)sock;
  write(fd, data, len);
}

int GetCookieData(const std::string& keyring, const std::string& id, std::string& cookie)
{
  int ret = 1;
  std::string keyringFile = getenv("HOME");
  keyringFile += "/.dbus-keyrings/" + keyring;
  FILE* file = fopen(keyringFile.c_str(), "r");
  if (!file)
    return 1;
  char buf[4096];
  while(fgets(buf, 4096, file))
  {
    if (strncmp(buf, id.c_str(), id.size()) == 0)
    {
      const char* bufEnd = buf + strlen(buf);
      const char* idEnd = &buf[id.size()];
      if (idEnd == bufEnd)
        break;

      const char* timeBegin = idEnd + 1;
      const char* timeEnd = std::find(timeBegin, bufEnd, ' ');
      if (timeEnd == bufEnd)
        break;

      const char* dataBegin = timeEnd + 1;
      const char* dataEnd = bufEnd - 1; // -1 for \n
      cookie = std::string(dataBegin, dataEnd);
      ret = 0;
      break;
    }
  }
  fclose(file);
  return ret;
}

int  ParseServerData(const char* data,
                     size_t len,
                     std::string& keyring,
                     std::string& id,
                     std::string& serverData)
{
  const char* dataEnd = data + len;

  const char* commandEnd = std::find(data, dataEnd, ' ');
  if (commandEnd == dataEnd)
    return 1;
  const char* hexDataBegin = commandEnd + 1;
  const char* hexDataEnd = std::find(hexDataBegin, dataEnd, '\r');

  std::string hexData(hexDataBegin, hexDataEnd);
  std::vector<uint8_t> decodedData;
  if (adbus::HexDecode(hexData.c_str(), hexData.size(), &decodedData))
    return 1;

  const char* decodedDataBegin = (const char*)&decodedData[0];
  const char* decodedDataEnd   = (const char*)&*decodedData.end();


  const char* keyringEnd = std::find(decodedDataBegin, decodedDataEnd, ' ');
  if (keyringEnd == decodedDataEnd)
    return 1;
  keyring = std::string(decodedDataBegin, keyringEnd);

  const char* idBegin = keyringEnd + 1;
  const char* idEnd = std::find(idBegin, decodedDataEnd, ' ');
  if (idEnd == decodedDataEnd)
    return 1;
  id = std::string(idBegin, idEnd);

  const char* serverDataBegin = idEnd + 1;
  serverData = std::string(serverDataBegin, decodedDataEnd);

  return 0;
}

void GenerateReply(const std::string& serverData, const std::string& cookie,
                   std::string& localData, std::string& reply)
{
  uint8_t clientData[32];
  for (int i = 0; i < 32; ++i)
    clientData[i] = rand() & 0xFF;

  adbus::HexEncode(&clientData[0], 32, &localData);

  std::string data = serverData
                   + ':'
                   + localData
                   + ':'
                   + cookie;

  SHA1 sha;
  sha.addBytes(data.c_str(), (int)data.size());

  adbus::HexEncode(sha.getDigest(), 20, &reply);
}

#define SEND(x) write(sock, x, strlen(x))

int main(int argc, char* argv[])
{
  int err = 0;
  ssize_t r;
  uint8_t buf[4096];
  int sock = tcpConnect("localhost", "12345");
  std::string keyring, id, serverData, cookie, localData, reply;

  char euidBuf[32];
  snprintf(euidBuf, 31, "%d", geteuid());
  euidBuf[31] = '\0';
  std::string authString;
  adbus::HexEncode((const uint8_t*)euidBuf, strlen(euidBuf), &authString);
  authString = "AUTH ADBUS_COOKIE_SHA1 " + authString + "\r\n";

  write(sock, "\0", 1);
  write(sock, authString.c_str(), authString.size());
  r = read(sock, buf, 4096);
  if (r < 0)
    return 1;

  if (ParseServerData((const char*)&buf[0], r, keyring, id, serverData))
    return 1;

  if (GetCookieData(keyring, id, cookie))
    return 1;

  GenerateReply(serverData, cookie, localData, reply);

  std::string fullReply = localData + ' ' + reply;
  std::string encodedFullReply;
  adbus::HexEncode((const uint8_t*) fullReply.c_str(), fullReply.size(), &encodedFullReply);

  encodedFullReply = "DATA " + encodedFullReply + "\r\n";

  write(sock, encodedFullReply.c_str(), encodedFullReply.size());

  r = read(sock, buf, 4096);

  SEND("BEGIN\r\n");

  adbus::Connection connection;
  connection.setSendCallback(&SendData, (void*)&sock);

  SomeRandomClass object;
  object.registerInterfaces(connection.addObject("/"));

  connection.connectToBus();

  while(!err)
  {
    r = read(sock, buf, 4096);
    if (r < 0)
    {
      fprintf(stderr, "%s\n", strerror(errno));
      err = 1;
    }
    err = connection.appendInputData(buf, r);
  }


  return err;

}
