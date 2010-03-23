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

#pragma once

#include "internal.h"

DILIST_INIT(Bind, adbus_ConnBind);
DHASH_MAP_INIT_STRSZ(Bind, adbus_ConnBind*);

struct adbus_ConnBind
{
    d_IList(Bind)           fl;
    adbusI_ObjectNode*      node;
    adbus_Interface*        interface;
    void*                   cuser2;
    adbus_ProxyMsgCallback  proxy;
    void*                   puser;
    adbus_Callback          release[2];
    void*                   ruser[2];
    adbus_ProxyCallback     relproxy;
    void*                   relpuser;
};

ADBUSI_FUNC adbus_ConnBind* adbusI_createBind(adbusI_ObjectTree* t, adbusI_ObjectNode* n, adbus_Bind* b);
ADBUSI_FUNC void adbusI_removeBind(adbus_ConnBind* b);
ADBUSI_FUNC void adbusI_freeBind(adbusI_ObjectNode* n);


DILIST_INIT(ObjectNode, adbusI_ObjectNode*);
DHASH_MAP_INIT_STRSZ(ObjectNode, adbusI_ObjectNode*);

struct adbusI_ObjectNode
{
    int                     ref;
    dh_strsz_t              path;
    d_Hash(Bind)            binds;
    d_IList(ObjectNode)     children;
    adbusI_ObjectNode*      parent;
};

ADBUSI_FUNC void adbusI_refObjectNode(adbusI_ObjectNode* n);
ADBUSI_FUNC void adbusI_derefObjectNode(adbusI_ObjectNode* n);
ADBUSI_FUNC void adbusI_removeObjectNode(adbusI_ObjectTree* p, adbusI_ObjectNode* n);
ADBUSI_FUNC void adbusI_freeObjectNode(adbusI_ObjectNode* n);

struct adbusI_ObjectTree
{
    d_Hash(ObjectNode)      lookup;
    d_IList(Bind)           binds;
};

// Object node comes pre-refed
ADBUSI_FUNC adbusI_ObjectNode* adbusI_getObjectNode(adbusI_ObjectTree* t, const char* path, size_t sz);
ADBUSI_FUNC void adbusI_removeObjectNodes(adbusI_ObjectTree* p, adbusI_ObjectNode* n);
ADBUSI_FUNC void adbusI_freeObjectTree(adbusI_ObjectTree* p);
