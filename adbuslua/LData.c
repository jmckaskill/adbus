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

#include "LData.h"

#include "adbus/User.h"

#include <assert.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void Unref(lua_State* L, int ref)
{
    if (!L || !ref)
        return;
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

static void FreeData(struct ADBusUser* user)
{
    if (!user)
        return;
    struct LADBusData* u = (struct LADBusData*) user;
    Unref(u->L, u->callback);
    Unref(u->L, u->argument);
    Unref(u->L, u->connection);
    Unref(u->L, u->interface);
    Unref(u->L, u->propertyType);
    Unref(u->L, u->returnSignature);
    free(u);
}

// ----------------------------------------------------------------------------

#define NEW(STRUCT) ((STRUCT*) calloc(1, sizeof(STRUCT)))

struct LADBusData* LADBusCreateData()
{
    struct LADBusData* data = NEW(struct LADBusData);
    data->header.free  = &FreeData;
    return data;
}

// ----------------------------------------------------------------------------

void LADBusPushRef(lua_State* L, int ref)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

// ----------------------------------------------------------------------------

int LADBusGetRef(lua_State* L, int index)
{
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// ----------------------------------------------------------------------------

int LADBusCopyRef(lua_State* L, int ref)
{
    LADBusPushRef(L, ref);
    int new_ref = LADBusGetRef(L, -1);
    lua_pop(L, 1);
    return new_ref;
}


