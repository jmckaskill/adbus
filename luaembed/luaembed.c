/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */


#ifdef __cplusplus
extern "C"{
#endif

#include "lua.h"
#include "lauxlib.h"
#include <string.h>
#include <stdint.h>
#include "luaembed.auto.h"

#ifdef WIN32
    __declspec(dllexport) int luaopen_luaembed(lua_State* L);
#else
    extern int luaopen_luaembed(lua_State* L);
#endif

static const char luaembed_handle[] = "luaembed";

static int luaembed_require(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    return luaembed_load(L, name);
}

int luaopen_luaembed(lua_State* L)
{
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1))
        return 0;
    lua_getfield(L, -1, "loaders");
    if (!lua_istable(L, -1))
        return 0;

    size_t n = lua_objlen(L, -1);
    lua_pushcfunction(L, &luaembed_require);
    lua_rawseti(L, -2, (int) (n + 1));

    lua_pop(L, 2);
    return 0;
}

#ifdef __cplusplus
}
#endif

