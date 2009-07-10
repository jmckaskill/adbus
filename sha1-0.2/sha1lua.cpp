#include "sha1.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

static int sha1_digest(lua_State* L)
{
  SHA1 sha;
  size_t size;
  const char* data = luaL_checklstring(L, 1, &size);
  sha.addBytes(data, size);

  unsigned char* digest = sha.getDigest();
  int digestlen = 20;
  lua_pushlstring(L, (const char*) digest, digestlen);
  return 1;
}


static const luaL_Reg kReg[] = {
  {"digest", &sha1_digest},
  {NULL, NULL},
};

extern "C" 
{
  #ifdef WIN32
  __declspec(dllexport)
  #endif
  int luaopen_sha1(lua_State* L);
}

#ifdef WIN32
__declspec(dllexport)
#endif
int luaopen_sha1(lua_State* L)
{
  luaL_register(L, "sha1", kReg);
  return 1;
}
