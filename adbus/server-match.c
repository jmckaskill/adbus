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

#include "server.h"

DVECTOR_INIT(Argument, adbus_Argument)

/* -------------------------------------------------------------------------- */

static int IsArgKey(const char* beg, const char* end, int* num)
{
    if (end - beg < 3 || strncmp(beg, "arg", 3) != 0)
    {
        return 0;
    }
    /* arg0 - arg9 */
    else if (   end - beg == 4
            &&  '0' <= beg[3] && beg[3] <= '9')
    {
        *num = (beg[3] - '0');
        return 1;
    }
    /* arg10 - arg63 */
    else if (   end - beg == 5
            &&  '0' <= beg[3] && beg[3] <= '9'
            &&  '0' <= beg[4] && beg[4] <= '9')
    {
        *num = (beg[3] - '0') * 10
             + (beg[4] - '0');
        return *num <= 63;
    }
    else
    {
        return 0;
    }
}

int adbusI_serv_addMatch(adbusI_ServerMatchList* list, const char* mstr, size_t len)
{
    adbusI_ServerMatch* m = (adbusI_ServerMatch*) calloc(1, sizeof(adbusI_ServerMatch) + len);
    d_Vector(Argument) args;
    const char *line, *end;

    ZERO(args);
    memcpy(m->data, mstr, len);
    m->size = len;

    line = m->data;
    end  = m->data + len;
    while (*line && line < end) {
        const char *keyb, *keye, *valb, *vale;
        int argnum;

        /* Look for a key/value pair "<key>='<value>',"
         * Comma is optional for the last key/value pair
         */
        keyb = line;
        keye = (const char*) memchr(keyb, '=', end - keyb);
        /* keye needs to be followed by ' */
        if (!keye || keye + 1 >= end || keye[1] != '\'')
            goto error;

        valb = keye+2;
        vale = (const char*) memchr(valb, '\'', end - valb);
        /* vale is either the last character or followed by a , */
        if (!vale || (vale + 1 != end && vale[1] != ',' ))
            goto error;

        line = vale + 2;

#define MATCH(BEG, END, STR)                        \
        (   END - BEG == sizeof(STR) - 1            \
         && memcmp(BEG, STR, END - BEG) == 0)

        if (MATCH(keyb, keye, "type")) {
            if (MATCH(valb, vale, "signal")) {
                m->m.type = ADBUS_MSG_SIGNAL;
            } else if (MATCH(valb, vale, "method_call")) {
                m->m.type = ADBUS_MSG_METHOD;
            } else if (MATCH(valb, vale, "method_return")) {
                m->m.type = ADBUS_MSG_RETURN;
            } else if (MATCH(valb, vale, "error")) {
                m->m.type = ADBUS_MSG_ERROR;
            } else {
                goto error;
            }

        } else if (MATCH(keyb, keye, "sender")) {
            m->m.sender           = valb;
            m->m.senderSize       = (int) (vale - valb);

        } else if (MATCH(keyb, keye, "interface")) {
            m->m.interface        = valb;
            m->m.interfaceSize    = (int) (vale - valb);

        } else if (MATCH(keyb, keye, "member")) {
            m->m.member           = valb;
            m->m.memberSize       = (int) (vale - valb);

        } else if (MATCH(keyb, keye, "path")) {
            m->m.path             = valb;
            m->m.pathSize         = (int) (vale - valb);

        } else if (MATCH(keyb, keye, "destination")) {
            m->m.destination      = valb;
            m->m.destinationSize  = (int) (vale - valb);

        } else if (IsArgKey(keyb, keye, &argnum)) {
            adbus_Argument* a;
            int toadd = argnum + 1 - (int) dv_size(&args);
            if (toadd > 0) {
                adbus_Argument* a = dv_push(Argument, &args, toadd);
                adbus_arg_init(a, toadd);
            }

            a = &dv_a(&args, argnum);
            a->value = valb;
            a->size  = (int) (vale - valb);
        }

    }

    m->m.argumentsSize = dv_size(&args);
    m->m.arguments     = dv_release(Argument, &args);

    dil_insert_after(ServerMatch, &list->list, m, &m->hl);
    return 0;

error:
    dv_free(Argument, &args);
    free(m);
    return -1;
}

/* -------------------------------------------------------------------------- */

int adbusI_serv_removeMatch(adbusI_ServerMatchList* list, const char* mstr, size_t len)
{
    adbusI_ServerMatch* m;
    DIL_FOREACH (ServerMatch, m, &list->list, hl) {
        if (len == m->size && memcmp(mstr, m->data, len) == 0) {
            dil_remove(ServerMatch, m, &m->hl);
            free(m->m.arguments);
            free(m);
            return 0;
        }
    }
    return -1;
}

/* -------------------------------------------------------------------------- */

void adbusI_serv_freeMatches(adbusI_ServerMatchList* list)
{
    adbusI_ServerMatch* m;
    DIL_FOREACH (ServerMatch, m, &list->list, hl) {
        dil_remove(ServerMatch, m, &m->hl);
        free(m->m.arguments);
        free(m);
    }
}

/* -------------------------------------------------------------------------- */

adbus_Bool adbusI_serv_matches(adbusI_ServerMatchList* list, adbus_Message* msg)
{
    adbusI_ServerMatch* match;
    DIL_FOREACH (ServerMatch, match, &list->list, hl) {
        if (adbusI_matchesMessage(&match->m, msg)) {
            return 1;
        }
    }
    return 0;
}

