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

DILIST_INIT(Bind, adbus_ConnBind)
DHASH_MAP_INIT_STRSZ(Bind, adbus_ConnBind*)

struct adbus_ConnBind
{
    d_IList(Bind)           hl;
    adbusI_ObjectNode*      node;
    adbus_Bind              b;
};

DLIST_INIT(ObjectNode, adbusI_ObjectNode)
DHASH_MAP_INIT_STRSZ(ObjectNode, adbusI_ObjectNode*)

struct adbusI_ObjectNode
{
    d_List(ObjectNode)      hl;
    int                     ref;
    dh_strsz_t              path;
    d_Hash(Bind)            binds;
    d_List(ObjectNode)      children;
    adbusI_ObjectNode*      parent;
    adbusI_ObjectTree*      tree;
    adbus_ConnBind*         introspectable;
    adbus_ConnBind*         properties;
};

struct adbusI_ObjectTree
{
    d_Hash(ObjectNode)      lookup;
    d_IList(Bind)           list;
};

ADBUSI_FUNC adbus_ConnBind* adbusI_createBind(adbusI_ObjectTree* t, adbusI_ObjectNode* n, const adbus_Bind* b);
ADBUSI_FUNC void adbusI_freeBind(adbus_ConnBind* b);

ADBUSI_FUNC void adbusI_refObjectNode(adbusI_ObjectNode* n);
ADBUSI_FUNC void adbusI_derefObjectNode(adbusI_ObjectNode* n);

/* Object node does _not_ come pre-refed */
ADBUSI_FUNC adbusI_ObjectNode* adbusI_getObjectNode(adbus_Connection* c, dh_strsz_t path);
ADBUSI_FUNC void adbusI_freeObjectTree(adbusI_ObjectTree* t);

ADBUSI_FUNC int adbusI_dispatchMethod(adbus_Connection* c, adbus_CbData* d);

ADBUSI_FUNC int adbusI_introspect(adbus_CbData* d);

ADBUS_API void adbus_bind_init(adbus_Bind* bind);

ADBUS_API adbus_ConnBind* adbus_conn_bind(
        adbus_Connection*   connection,
        const adbus_Bind*   bind);

ADBUS_API void adbus_conn_unbind(
        adbus_Connection*   connection,
        adbus_ConnBind*     bind);

ADBUS_API adbus_Interface* adbus_conn_interface(
        adbus_Connection*       connection,
        const char*             path,
        int                     pathSize,
        const char*             interface,
        int                     interfaceSize,
        adbus_ConnBind**        bind);

ADBUS_API adbus_Member* adbus_conn_method(
        adbus_Connection*       connection,
        const char*             path,
        int                     pathSize,
        const char*             method,
        int                     methodSize,
        adbus_ConnBind**        bind);

