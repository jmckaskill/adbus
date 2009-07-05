// vim: ts=4 sw=4 sts=4 et
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

#include "Data.h"

#include "adbus/User.h"

#include <assert.h>
#include <lauxlib.h>
#include <stdlib.h>


void LADBusSetupData(const struct LADBusData* data, struct ADBusUser* user)
{
    ADBusUserInit(user);
    user->data = malloc(sizeof(struct LADBusData));
    user->size = sizeof(struct LADBusData);
    user->clone = &LADBusCloneData;
    user->free  = &LADBusFreeData;
    memcpy(user->data, data, sizeof(struct LADBusData));
}

// ----------------------------------------------------------------------------

void LADBusCloneData(const struct ADBusUser* from, struct ADBusUser* to)
{
    const struct LADBusData* dfrom = LADBusCheckData(from);
    LADBusSetupData(dfrom, to); 
    struct LADBusData* dto = (struct LADBusData*) to->data;
    if (dfrom->L) {
        for (int i = 0; i < 3; ++i) {
            if (!dfrom->ref[i])
                continue;
            lua_rawgeti(dfrom->L, LUA_REGISTRYINDEX, dfrom->ref[0]);
            dto->ref[0] = luaL_ref(dfrom->L, LUA_REGISTRYINDEX);
        }
    }
}

// ----------------------------------------------------------------------------

void LADBusFreeData(struct ADBusUser* user)
{
    const struct LADBusData* u = LADBusCheckData(user);
    if (u->L) {
        for (int i = 0; i < 3; ++i) {
            if (u->ref[i])
                luaL_unref(u->L, LUA_REGISTRYINDEX, u->ref[i]);
        }
    }

    free(user->data);
}

// ----------------------------------------------------------------------------

const struct LADBusData* LADBusCheckData(const struct ADBusUser* user)
{
    assert(user->size == sizeof(struct LADBusData) && user->data);
    return (struct LADBusData*) user->data;
}

// ----------------------------------------------------------------------------

