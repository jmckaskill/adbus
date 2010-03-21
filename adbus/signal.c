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

#include <adbus.h>

#include "misc.h"
#include "interface.h"

/** \struct adbus_Signal
 *  \brief Helper class to emit signals from C code.
 *
 *  adbus_Signal can be bound to the same member on any number of paths on any
 *  number of connections. The workflow to emit a signal is:
 *  -# Grab a message factory using adbus_sig_msg().
 *  -# Append arguments to the factory.
 *  -# Send off the signals messages using adbus_sig_send().
 *
 *  \note If there are no arguments a single call to adbus_sig_emit() will
 *  suffice.
 *
 *  For example:
 *  \code
 *  void EmitSignal()
 *  {
 *      adbus_MsgFactory* m = adbus_sig_msg(my_signal);
 *      adbus_msg_string(m, "foo", -1);
 *      adbus_sig_emit(my_signal);
 *  }
 *  \endcode
 *
 */

/* ------------------------------------------------------------------------- */

struct Bind
{
    adbus_Connection*   connection;
    char*               path;
    size_t              pathSize;
};

DVECTOR_INIT(Bind, struct Bind);

struct adbus_Signal
{
    /** \privatesection */
    adbus_MsgFactory*   message;
    d_Vector(Bind)      binds;
    adbus_Member*       member;
};

/* ------------------------------------------------------------------------- */

/** Create a new signal for the given member
 *  \relates adbus_Signal
 */
adbus_Signal* adbus_sig_new(adbus_Member* mbr)
{
    adbus_Signal* s = NEW(adbus_Signal);
    s->message  = adbus_msg_new();
    s->member = mbr;
    adbus_iface_ref(mbr->interface);
    return s;
}

/* ------------------------------------------------------------------------- */

/** Frees the signal
 *  \relates adbus_Signal
 */
void adbus_sig_free(adbus_Signal* s)
{
    if (s) {
        adbus_sig_reset(s);
        dv_free(Bind, &s->binds);
        adbus_iface_deref(s->member->interface);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */

/** Removes all current bindings
 *  \relates adbus_Signal
 */
void adbus_sig_reset(adbus_Signal* s)
{
    adbus_msg_free(s->message);
    for (size_t i = 0; i < dv_size(&s->binds); i++) {
        free(dv_a(&s->binds, i).path);
    }
    dv_clear(Bind, &s->binds);
}

/* ------------------------------------------------------------------------- */

/** Adds a path/connection to emit on
 *  \relates adbus_Signal
 */
void adbus_sig_bind(
        adbus_Signal*       s,
        adbus_Connection*   c,
        const char*         path,
        int                 pathSize)
{
    if (pathSize < 0)
        pathSize = strlen(path);

    struct Bind* b  = dv_push(Bind, &s->binds, 1);
    b->path         = adbusI_strndup(path, pathSize);
    b->pathSize     = pathSize;
    b->connection   = c;
}

/* ------------------------------------------------------------------------- */

/** Resets and returns the internal message factory.
 *  \relates adbus_Signal
 *
 *  \note This sets the argument signature on the factory.
 */
adbus_MsgFactory* adbus_sig_msg(adbus_Signal* s)
{
    adbus_MsgFactory* m = s->message;
    adbus_msg_reset(m);
    adbus_msg_setsig(m, ds_cstr(&s->member->argsig), ds_size(&s->member->argsig));
    return m;
}

/* ------------------------------------------------------------------------- */

/** Emits the signal on all bound path/connection combos.
 *  \relates adbus_Signal
 *
 *  If there are any arguments they should have already been added by getting
 *  the message factory with adbus_sig_msg() and appending them.
 */
void adbus_sig_emit(adbus_Signal* s)
{
    adbus_MsgFactory* m = s->message;
    adbus_Interface* i = s->member->interface;

    adbus_msg_end(m);
    adbus_msg_setmember(m, s->member->name.str, s->member->name.sz);
    adbus_msg_setinterface(m, i->name.str, i->name.sz);

    for (size_t i = 0; i < dv_size(&s->binds); i++) {
        struct Bind* b = &dv_a(&s->binds, i);

        adbus_Connection* c = b->connection;

        adbus_msg_settype(m, ADBUS_MSG_SIGNAL);
        adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);
        adbus_msg_setserial(m, adbus_conn_serial(c));
        adbus_msg_setpath(m, b->path, b->pathSize);

        adbus_msg_send(m, c);
    }

    // Reset the message
    adbus_sig_msg(s);
}



