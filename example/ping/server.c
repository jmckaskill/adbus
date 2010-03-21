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
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#endif

#undef interface

#ifdef _MSC_VER
#   define strdup _strdup
#endif

static int quit = 0;

static int Quit(adbus_CbData* d)
{
    quit = 1;
    return 0;
}

static int Ping(adbus_CbData* d)
{
    const char* ping = adbus_check_string(d, NULL);
    adbus_check_end(d);

    if (d->ret) {
        adbus_msg_setsig(d->ret, "s", -1);
        adbus_msg_string(d->ret, ping, -1);
    }
    return 0;
}

struct ReplyData
{
    adbus_Connection*   connection;
    adbus_State*        state;
    int                 replies;
    int                 ref;
    uint32_t            serial;
    char*               sender;
};

static void FreeReply(struct ReplyData* r)
{
    adbus_state_free(r->state);
    free(r->sender);
    free(r);
}

static void ReleaseReply(void* u)
{
    struct ReplyData* r = (struct ReplyData*) u;
    if (--r->ref == 0)
        FreeReply(r);
}

static int Reply(adbus_CbData* d)
{
    struct ReplyData* r = (struct ReplyData*) d->user1;
    if (d->msg->type == ADBUS_MSG_ERROR) {
        adbus_MsgFactory* msg = adbus_msg_new();

        adbus_msg_settype(msg, ADBUS_MSG_ERROR);
        adbus_msg_setdestination(msg, r->sender, -1);
        adbus_msg_setreply(msg, r->serial);
        adbus_msg_seterror(msg, d->msg->error, -1);

        if (*d->msg->signature == 's') {
            const char* sig = adbus_check_string(d, NULL);
            adbus_msg_setsig(msg, "s", -1);
            adbus_msg_string(msg, sig, -1);
        }

        adbus_msg_send(msg, r->connection);
        adbus_msg_free(msg);
        FreeReply(r);

    } else if (--r->replies == 0) {
        adbus_MsgFactory* msg = adbus_msg_new();

        adbus_msg_settype(msg, ADBUS_MSG_RETURN);
        adbus_msg_setdestination(msg, r->sender, -1);
        adbus_msg_setreply(msg, r->serial);

        adbus_msg_send(msg, r->connection);
        adbus_msg_free(msg);
        FreeReply(r);

    }
    return 0;
}

#define NEW(T) ((T*) calloc(1, sizeof(T)))

static int Call(adbus_CbData* d)
{
    int count           = adbus_check_u32(d);
    const char* service = adbus_check_string(d, NULL);
    const char* path    = adbus_check_objectpath(d, NULL);
    const char* method  = adbus_check_string(d, NULL);

    adbus_IterVariant data;
    adbus_check_beginvariant(d, &data);
    adbus_check_value(d);
    adbus_check_endvariant(d, &data);

    struct ReplyData* r = NEW(struct ReplyData);
    r->ref          = count;
    r->replies      = count;
    r->state        = adbus_state_new();
    r->connection   = d->connection;
    r->sender       = strdup(d->msg->sender);

    adbus_Proxy* p = adbus_proxy_new(r->state);
    adbus_proxy_init(p, d->connection, service, -1, path, -1);
    for (int i = 0; i < count; i++) {
        adbus_Call f;
        adbus_call_method(p, &f, method, -1);

        f.callback      = &Reply;
        f.cuser         = r;
        f.error         = &Reply;
        f.euser         = r;
        f.release[0]    = &ReleaseReply;
        f.ruser[0]      = r;

        adbus_msg_setsig(f.msg, data.sig, -1);
        adbus_msg_append(f.msg, data.data, data.size);

        adbus_call_send(p, &f);
    }

    d->ret = NULL;

    return 0;
}

static int CallNoReply(adbus_CbData* d)
{
    int count           = adbus_check_u32(d);
    const char* service = adbus_check_string(d, NULL);
    const char* path    = adbus_check_objectpath(d, NULL);
    const char* method  = adbus_check_string(d, NULL);

    adbus_IterVariant data;
    adbus_check_beginvariant(d, &data);
    adbus_check_value(d);
    adbus_check_endvariant(d, &data);

    adbus_State* s = adbus_state_new();
    adbus_Proxy* p = adbus_proxy_new(s);
    adbus_proxy_init(p, d->connection, service, -1, path, -1);

    for (int i = 0; i < count; i++) {
        adbus_Call f;
        adbus_call_method(p, &f, method, -1);

        adbus_msg_setsig(f.msg, data.sig, -1);
        adbus_msg_append(f.msg, data.data, data.size);

        adbus_call_send(p, &f);
    }

    adbus_proxy_free(p);
    adbus_state_free(s);

    return 0;
}

static adbus_ssize_t Send(void* d, adbus_Message* m)
{ return send(*(adbus_Socket*) d, m->data, m->size, 0); }

#define RECV_SIZE 64 * 1024

int main()
{

#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
        abort();
#endif

    adbus_Buffer* buf = adbus_buf_new();
    adbus_Socket sock = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sock == ADBUS_SOCK_INVALID || adbus_sock_cauth(sock, buf))
        abort();

    adbus_Connection* c = adbus_conn_new();
    adbus_conn_setsender(c, &Send, &sock);

    adbus_Interface* i = adbus_iface_new("nz.co.foobar.adbus.PingTest", -1);

    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    mbr = adbus_iface_addmethod(i, "Ping", -1);
    adbus_mbr_setmethod(mbr, &Ping, NULL);
    adbus_mbr_argsig(mbr, "s", -1);
    adbus_mbr_retsig(mbr, "s", -1);

    mbr = adbus_iface_addmethod(i, "Call", -1);
    adbus_mbr_setmethod(mbr, &Call, NULL);
    adbus_mbr_argsig(mbr, "usosv", -1);
    adbus_mbr_argname(mbr, "repeat", -1);
    adbus_mbr_argname(mbr, "service", -1);
    adbus_mbr_argname(mbr, "path", -1);
    adbus_mbr_argname(mbr, "method", -1);
    adbus_mbr_argname(mbr, "data", -1);

    mbr = adbus_iface_addmethod(i, "CallNoReply", -1);
    adbus_mbr_setmethod(mbr, &CallNoReply, NULL);
    adbus_mbr_argsig(mbr, "usosv", -1);
    adbus_mbr_argname(mbr, "repeat", -1);
    adbus_mbr_argname(mbr, "service", -1);
    adbus_mbr_argname(mbr, "path", -1);
    adbus_mbr_argname(mbr, "method", -1);
    adbus_mbr_argname(mbr, "data", -1);

    adbus_Bind b;
    adbus_bind_init(&b);
    b.interface = i;
    b.path = "/";
    adbus_conn_bind(c, &b);

    adbus_conn_connect(c, NULL, NULL);

    adbus_State* s = adbus_state_new();
    adbus_Proxy* p = adbus_proxy_new(s);
    adbus_proxy_init(p, c, "org.freedesktop.DBus", -1, "/", -1);

    adbus_Call f;
    adbus_call_method(p, &f, "RequestName", -1);
    adbus_msg_setsig(f.msg, "su", -1);
    adbus_msg_string(f.msg, "nz.co.foobar.adbus.PingServer", -1);
    adbus_msg_u32(f.msg, 0);
    adbus_call_send(p, &f);

    adbus_proxy_free(p);

    while(!quit) {
        char* dest = adbus_buf_recvbuf(buf, RECV_SIZE);
        adbus_ssize_t recvd = recv(sock, dest, RECV_SIZE, 0);
        adbus_buf_recvd(buf, RECV_SIZE, recvd);
        if (recvd <= 0)
            abort();

        if (adbus_conn_parse(c, buf))
            abort();
    }

    adbus_conn_free(c);
    adbus_iface_free(i);
    adbus_buf_free(buf);

    return 0;
}

