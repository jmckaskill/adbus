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

/* Allow use of features specific to Windows XP or later. */
#define WINVER 0x0500
#define _WIN32_WINNT 0x0501

#include "auth.h"
#include "sha1.h"

#include <stdio.h>

/** \struct adbus_Auth
 *  \brief Minimal SASL client and server to connect to DBUS
 *  
 *  \note The auth module is not tied to the adbus_Connection or adbus_Server
 *  modules and can be used independently and can be replaced if needed.
 *
 *  \note The auth module does _not_ send the leading null byte. This must be
 *  sent seperately.
 *
 *  \note If using adbus_auth_parse(), you should use the same buffer for the
 *  connnection or remote parsing, as received data after the auth is complete
 *  is left in the buffer.
 *
 *  The adbus_Auth is used for both client and server connections. The
 *  client specific function begin with adbus_cauth_, server specific
 *  adbus_sauth_, and common adbus_auth_.
 *
 *  The auth module is designed to be extendable by allowing the user of the
 *  API to manually filter out lines and handle them themselves. For example
 *  you could add a filter for "STARTTLS" which would then initiate TLS on the
 *  connection.
 *
 *  \section client Client Authentication
 *
 *  The workflow for client auth is:
 *  -# Create the auth structure using adbus_cauth_new().
 *  -# Setup auth mechanisms for example adbus_cauth_external().
 *  -# Start the auth with adbus_cauth_start().
 *  -# Feed responses from the remote using adbus_auth_parse() or
 *  adbus_auth_line() until they return a non-zero value.
 *  -# Free the auth structure.
 *
 *  For example (this is the actual code for adbus_sock_cauth):
 *  \code
 *  int Send(void* user, const char* data, size_t sz)
 *  {
 *      adbus_Socket s = *(adbus_Socket*) user;
 *      return send(s, data, sz, 0);
 *  }
 *
 *  uint8_t Rand(void* user)
 *  {
 *      (void) user;
 *      return (uint8_t) rand();
 *  }
 *
 *  #define RECV_SIZE 1024
 *  int adbus_sock_cauth(adbus_Socket sock, adbus_Buffer* buffer)
 *  {
 *      int ret = 0;
 *
 *      adbus_Auth* a = adbus_cauth_new(&Send, &Rand, &sock);
 *      adbus_cauth_external(a);
 *
 *      if (send(sock, "\0", 1, 0) != 1)
 *          goto err;
 *
 *      if (adbus_cauth_start(a))
 *          goto err;
 *
 *      while (!ret) {
 *          // Try and get some data
 *          char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
 *          int read = recv(sock, dest, RECV_SIZE, 0);
 *          adbus_buf_recvd(buffer, RECV_SIZE, read);
 *          if (read < 0)
 *              goto err;
 *
 *          // Hand the received data off to the auth module
 *          ret = adbus_auth_parse(a, buffer);
 *          if (ret < 0)
 *              goto err;
 *      }
 *
 *      adbus_auth_free(a);
 *      return 0;
 *
 *  err:
 *      adbus_auth_free(a);
 *      return -1;
 *  }
 *  \endcode
 *
 *  \note If working with blocking BSD sockets, adbus_sock_cauth() performs
 *  the entire procudure.
 *
 *  \sa adbus_sock_cauth()
 *
 *
 *
 *  \section server Server Authentication
 *
 *  \note Server here refers to the bus server, not bus services.
 *
 *  The workflow for server auth is:
 *  -# Create the auth structure.
 *  -# Setup auth mechansims eg adbus_sauth_external.
 *  -# Feed requests from the remote using adbus_auth_parse() or
 *  adbus_auth_line() until they return a non-zero value.
 *  -# Free the auth structure.
 *
 *  \note There is no server equivalent of adbus_cauth_start(), as the SASL
 *  protocol is client initiated.
 *
 *  For example:
 *  \code
 *  adbus_Bool IsExternalValid(void* user, const char* id)
 *  {
 *      adbus_Socket s = *(adbus_Socket*) user;
 *      return IsSocketValidForId(s, id);
 *  }
 *
 *  int AuthRemote(adbus_Socket sock, adbus_Buffer* buffer)
 *  {
 *      int ret = 0;
 *
 *      adbus_Auth* a = adbus_sauth_new(&Send, &Rand, &sock);
 *      adbus_sauth_external(a, &IsExternalValid);
 *
 *      // Make sure we get the beginnig null correctly
 *      char buf[1];
 *      if (recv(sock, &buf, 1, 0) != 1 || buf[0] != '\0')
 *          goto err;
 *
 *      while (!ret) {
 *          // Get some data
 *          char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
 *          int read = recv(sock, dest, RECV_SIZE, 0);
 *          adbus_buf_recvd(buffer, RECV_SIZE, read);
 *          if (read < 0)
 *              goto err;
 *
 *          // Hand the received data off to the auth module
 *          ret = adbus_auth_parse(a, buffer);
 *          if (ret < 0)
 *              goto err;
 *      }
 *
 *      adbus_auth_free(a);
 *      return 0;
 *
 *  err:
 *      adbus_auth_free(a);
 *      return -1;
 *  }
 *  \endcode
 *
 *  
 */

/* -------------------------------------------------------------------------- */

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#include <sddl.h>

static void LocalId(d_String* id)
{
    HANDLE process_token = NULL;
    TOKEN_USER *token_user = NULL;
    DWORD n;
    PSID psid;
    LPTSTR stringsid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &process_token))
        goto failed;

    /* it should fail as it has no buffer */
    if (GetTokenInformation(process_token, TokenUser, NULL, 0, &n))
        goto failed;

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto failed;

    token_user = (TOKEN_USER*) alloca(n);
    if (!GetTokenInformation(process_token, TokenUser, token_user, n, &n))
        goto failed;

    psid = token_user->User.Sid;
    if (!IsValidSid (psid))
        goto failed;

    if (!ConvertSidToStringSid(psid, &stringsid))
        goto failed;

#ifdef UNICODE
    size_t sidlen = wcslen(stringsid);
    for (size_t i = 0; i < sidlen; ++i)
        ds_cat_char(id, (char) stringsid[i]);
#else
    ds_cat(id, stringsid);
#endif

    LocalFree(stringsid);

failed:
    if (process_token != NULL)
        CloseHandle(process_token);
}

#else

#include <unistd.h>
#include <sys/types.h>

static void LocalId(d_String* id)
{
    uid_t uid = geteuid();
    ds_cat_f(id, "%d", (int) uid);
}

#endif

/* -------------------------------------------------------------------------- */

static void HexDecode(d_String* out, const char* str, size_t size)
{
    size_t i;
    const uint8_t* ustr = (const uint8_t*) str;

    if (size % 2 != 0)
        return;

    for (i = 0; i < size / 2; ++i)
    {
        uint8_t hi = ustr[2*i];
        uint8_t lo = ustr[2*i + 1];

        if ('0' <= hi && hi <= '9') {
            hi = hi - '0';
        } else if ('a' <= hi && hi <= 'f') {
            hi = hi - 'a' + 10;
        } else if ('A' <= hi && hi <= 'F') {
            hi = hi - 'A' + 10;
        } else {
            return;
        }

        if ('0' <= lo && lo <= '9') {
            lo = lo - '0';
        } else if ('a' <= lo && lo <= 'f') {
            lo = lo - 'a' + 10;
        } else if ('A' <= lo && lo <= 'F') {
            lo = lo - 'A' + 10;
        } else {
            return;
        }

        ds_cat_char(out, (hi << 4) | lo);
    }
}

/* -------------------------------------------------------------------------- */

static void HexEncode(d_String* out, const char* data, size_t size)
{
    size_t i;
    const uint8_t* udata = (const uint8_t*) data;
    for (i = 0; i < size; ++i)
    {
        uint8_t hi = udata[i] >> 4;
        uint8_t lo = udata[i] & 0x0F;

        if (hi < 10) {
            hi += '0';
        } else {
            hi += 'a' - 10;
        }

        if (lo < 10) {
            lo += '0';
        } else {
            lo += 'a' - 10;
        }

        ds_cat_char(out, hi);
        ds_cat_char(out, lo);
    }
}

/* -------------------------------------------------------------------------- */
static int GetCookie(const char*  keyring,
                     const char*  id,
                     d_String*    cookie)
{
    char buf[4096];
    FILE* file = NULL;
    d_String keyringFile;
#ifdef _WIN32
    const char* home = getenv("userprofile");
#else
    const char* home = getenv("HOME");
#endif

    ZERO(keyringFile);

    if (home) {
        ds_cat(&keyringFile, home);
        ds_cat(&keyringFile, "/");
    }

    /* If no HOME env then we go off the current directory */
    ds_cat(&keyringFile, ".dbus-keyrings/");
    ds_cat(&keyringFile, keyring);

    file = fopen(ds_cstr(&keyringFile), "r");
    if (!file)
        goto end;

    while(fgets(buf, 4096, file))
    {
        /* Find the line that begins with id */
        if (strncmp(buf, id, strlen(id)) == 0)
        {
            const char *bufEnd, *idEnd, *timeBegin, *timeEnd, *dataBegin, *dataEnd;

            bufEnd = buf + strlen(buf);
            idEnd = buf + strlen(id);
            if (idEnd >= bufEnd)
                break;

            timeBegin = idEnd + 1;
            timeEnd = strchr(timeBegin, ' ');
            if (!timeEnd)
                break;

            dataBegin = timeEnd + 1;
            dataEnd = bufEnd - 1; /* -1 for \n */
            ds_set_n(cookie, dataBegin, dataEnd - dataBegin);
            break;
        }
    }

end:
    if (file)
        fclose(file);
    ds_free(&keyringFile);
    return ds_size(cookie) == 0;
}

/* -------------------------------------------------------------------------- */
static int ParseServerData(const char* data,
                           d_String* keyring,
                           d_String* id,
                           d_String* serverData)
{
    const char *keyringEnd, *idBegin, *idEnd;

    keyringEnd = strchr(data, ' ');
    if (!keyringEnd)
        return 1;

    if (keyring)    
        ds_set_n(keyring, data, keyringEnd - data);
    
    idBegin = keyringEnd + 1;
    idEnd = strchr(idBegin, ' ');
    if (!idEnd)
        return 1;
    
    if (id)
        ds_set_n(id, idBegin, idEnd - idBegin);
    if (serverData)
        ds_set(serverData, idEnd + 1);

    return 0;
}








/* -------------------------------------------------------------------------- */
/** Free an auth structure 
 *  \relates adbus_Auth
 */
void adbus_auth_free(adbus_Auth* a)
{
    if (a) {
        ds_free(&a->id);
        ds_free(&a->okCmd);
        adbus_buf_free(a->buf);
        free(a);
    }
}




/* -------------------------------------------------------------------------- */
/** Create an auth structure for server use
 *  \relates adbus_Auth
 */
adbus_Auth* adbus_sauth_new(
        adbus_SendCallback  send,
        adbus_RandCallback  rand,
        void*               data)
{
    adbus_Auth* a   = NEW(adbus_Auth);
    a->buf          = adbus_buf_new();
    a->server       = 1;
    a->external     = AUTH_NOT_SUPPORTED;
    a->send         = send;
    a->rand         = rand;
    a->data         = data;

    ds_set(&a->okCmd, "OK 1234deadbeef\r\n");

    return a;
}

/* -------------------------------------------------------------------------- */
/** Sets the server uuid
 *  \relates adbus_Auth
 */
void adbus_sauth_setuuid(adbus_Auth* a, const char* uuid)
{
    ds_clear(&a->okCmd);
    ds_cat(&a->okCmd, "OK ");
    HexEncode(&a->okCmd, uuid, strlen(uuid));
    ds_cat(&a->okCmd, "\r\n");
}

/* -------------------------------------------------------------------------- */
/** Sets up a server auth for the external auth mechanism
 *  \relates adbus_Auth
 *
 *  The supplied callback is called to see if the remote can authenticate
 *  using the external auth mechanism. The user data supplied to the callback
 *  is the user data supplied in adbus_sauth_new().
 *
 */
void adbus_sauth_external(
        adbus_Auth*             a,
        adbus_ExternalCallback  cb)
{
    assert(a->server && a->external == AUTH_NOT_SUPPORTED);
    a->external   = AUTH_NOT_TRIED;
    a->externalcb = cb;
}




/* -------------------------------------------------------------------------- */
/** Create an auth structure for client use
 *  \relates adbus_Auth
 */
adbus_Auth* adbus_cauth_new(
        adbus_SendCallback  send,
        adbus_RandCallback  rand,
        void*               data)
{
    adbus_Auth* a   = NEW(adbus_Auth);
    a->buf          = adbus_buf_new();
    a->server       = 0;
    a->external     = AUTH_NOT_SUPPORTED;
    a->send         = send;
    a->rand         = rand;
    a->data         = data;

    LocalId(&a->id);

    return a;
}

/* -------------------------------------------------------------------------- */
/** Sets up a client auth to try the external auth mechanism
 *  \relates adbus_Auth
 */
 void adbus_cauth_external(adbus_Auth* a)
{
    assert(!a->server && a->external == AUTH_NOT_SUPPORTED);
    a->external   = AUTH_NOT_TRIED;
}

/* -------------------------------------------------------------------------- */
static int ClientReset(adbus_Auth* a);

/** Initiates the client auth
 *  \relates adbus_Auth
 */
int adbus_cauth_start(adbus_Auth* a)
{ return ClientReset(a); }





/* -------------------------------------------------------------------------- */
static void RandomData(adbus_Auth* a, d_String* str)
{
    size_t i;
    for (i = 0; i < 64; i++) {
        ds_cat_char(str, a->rand(a->data));
    }
}

/* -------------------------------------------------------------------------- */
static int Send(adbus_Auth* a, const char* data)
{
    int sent;
    size_t sz = strlen(data);

    ADBUSI_LOG_DATA_2(data, sz, "sending (auth %s, %p)",
            ds_cstr(&a->id),
            (void*) a);

    sent = a->send(a->data, data, sz);
    if (sent < 0) {
        return -1;
    } else if (sent != (int) sz) {
        assert(0);
        return -1;
    } else {
        return 0;
    }
}




/* -------------------------------------------------------------------------- */
static int ServerReset(adbus_Auth* a)
{ 
    a->okSent = 0;
    return Send(a, "REJECTED EXTERNAL\r\n"); 
}

/* -------------------------------------------------------------------------- */
static int ServerOk(adbus_Auth* a)
{
    a->okSent = 1;
    return Send(a, ds_cstr(&a->okCmd));
}

/* -------------------------------------------------------------------------- */
static int ServerExternalInit(adbus_Auth* a)
{ 
    a->external = AUTH_BEGIN;
    if (a->externalcb == NULL || a->externalcb(a->data, ds_cstr(&a->id))) {
        return ServerOk(a);
    } else {
        return ServerReset(a);
    }
}

/* -------------------------------------------------------------------------- */
#define MATCH(STR1, SZ1, STR2) (SZ1 == strlen(STR2) && memcmp(STR1, STR2, SZ1) == 0)
static int ServerAuth(adbus_Auth* a, const char* mech, size_t mechsz, d_String* data)
{
    ds_set_s(&a->id, data);
    if (a->external == AUTH_NOT_TRIED && MATCH(mech, mechsz, "EXTERNAL")) {
        return ServerExternalInit(a);
    } else {
        return ServerReset(a);
    }
}





/* -------------------------------------------------------------------------- */
static int ClientCookie(adbus_Auth* a, d_String* data)
{
    int ret = 0;
    uint8_t* digest = NULL;
    adbusI_SHA1 sha;
    d_String keyring;   
    d_String keyringid; 
    d_String servdata;  
    d_String cookie;    
    d_String shastr;    
    d_String reply;     
    d_String replyarg;  
    d_String localdata; 

    ZERO(keyring);
    ZERO(keyringid);
    ZERO(servdata);
    ZERO(cookie);
    ZERO(shastr);
    ZERO(reply);
    ZERO(replyarg);
    ZERO(localdata);

    if (    ParseServerData(ds_cstr(data), &keyring, &keyringid, &servdata)
        ||  GetCookie(ds_cstr(&keyring), ds_cstr(&keyringid), &cookie))
    {
        ret = -1;
        goto end;
    }

    RandomData(a, &localdata);

    ds_cat_s(&shastr, &servdata);
    ds_cat(&shastr, ":");
    HexEncode(&shastr, ds_cstr(&localdata), ds_size(&localdata));
    ds_cat(&shastr, ":");
    ds_cat_s(&shastr, &cookie);

    adbusI_SHA1Init(&sha);
    adbusI_SHA1AddBytes(&sha, ds_cstr(&shastr), ds_size(&shastr));
    digest = adbusI_SHA1GetDigest(&sha);

    HexEncode(&replyarg, ds_cstr(&localdata), ds_size(&localdata));
    ds_cat(&replyarg, " ");
    HexEncode(&replyarg, (char*) digest, 20);

    ds_cat(&reply, "DATA ");
    HexEncode(&reply, ds_cstr(&replyarg), ds_size(&replyarg));
    ds_cat(&reply, "\r\n");

    ret = Send(a, ds_cstr(&reply));

end:
    ds_free(&keyring);
    ds_free(&keyringid);
    ds_free(&servdata);
    ds_free(&cookie);
    ds_free(&shastr);
    ds_free(&reply);
    ds_free(&replyarg);
    ds_free(&localdata);
    free(digest);
    return ret;
}

/* -------------------------------------------------------------------------- */
static int ClientReset(adbus_Auth* a)
{
    int ret = 0;
    d_String msg;
    ZERO(msg);

    if (a->external == AUTH_NOT_TRIED) {
        ds_cat(&msg, "AUTH EXTERNAL ");
        a->onData = NULL;
        a->external = AUTH_BEGIN;
    } else if (a->cookie == AUTH_NOT_TRIED) {
        ds_cat(&msg, "AUTH DBUS_COOKIE_SHA1 ");
        a->onData = &ClientCookie;
        a->cookie = AUTH_BEGIN;
    } else {
        ret = -1;
        goto end;
    }

    HexEncode(&msg, ds_cstr(&a->id), ds_size(&a->id));
    ds_cat(&msg, "\r\n");

    ret = Send(a, ds_cstr(&msg));
end:
    ds_free(&msg);
    return ret;
}

/* -------------------------------------------------------------------------- */
/*  Parses a line received from the remote.
 *  \return 1 on success
 *  \return 0 on continue
 *  \return -1 on error
 */
static int ProcessLine(adbus_Auth* a, const char* line, size_t len)
{
    const char *end, *cmdb, *cmde;
    size_t cmdsz;

    if (line[len - 1] == '\n')
        len--;
    if (line[len - 1] == '\r')
        len--;

    end = line + len;

    cmdb = line;
    cmde = (const char*) memchr(cmdb, ' ', end - cmdb);
    if (cmde == NULL)
        cmde = end;
    cmdsz = cmde - cmdb;

    if (a->server && cmde != end && MATCH(cmdb, cmdsz, "AUTH")) {
        const char* mechb = cmde + 1;
        const char* meche = (const char*) memchr(mechb, ' ', end - mechb);
        if (meche) {
            const char* datab = meche + 1;
            d_String data;
            int ret;

            ZERO(data);

            HexDecode(&data, datab, end - datab);

            ret = ServerAuth(a, mechb, meche - mechb, &data);
            ds_free(&data);
            return ret;

        } else {
            Send(a, "ERROR\r\n");
            return 0;
        }

    } else if (a->onData && MATCH(cmdb, cmdsz, "DATA")) {
        d_String data;
        int ret;

        ZERO(data);

        if (cmde != end) {
            const char* datab = line + sizeof("DATA");
            HexDecode(&data, datab, end - datab);
        }

        ret = a->onData(a, &data);
        ds_free(&data);
        return ret;
        
    } else if (a->server && MATCH(cmdb, cmdsz, "CANCEL")) {
        return ServerReset(a);

    } else if (a->server && a->okSent && MATCH(cmdb, cmdsz, "BEGIN")) {
        return 1;

    } else if (!a->server && MATCH(cmdb, cmdsz, "OK")) {
        Send(a, "BEGIN\r\n");
        return 1;

    } else if (!a->server && MATCH(cmdb, cmdsz, "REJECTED")) {
        return ClientReset(a);

    } else if (MATCH(cmdb, cmdsz, "ERROR")) {
        /* We have no way of handling these */
        return -1;

    } else {
        Send(a, "ERROR\r\n");
        return 0;
    }
}

/* -------------------------------------------------------------------------- */
/** Consumes the provided data 
 *  \relates adbus_Auth
 *
 *  \return -1 on error
 *  \return how much data was used (this will be \a sz if more data is needed)
 */
int adbus_auth_parse(adbus_Auth* a, const char* data, size_t sz, adbus_Bool* finished)
{
    int ret = 0;

    if (a->successful) {
        *finished = 1;
        return 0;
    }

    *finished = 0;

    ADBUSI_LOG_DATA_2(data, sz, "received (auth %s, %p)",
            ds_cstr(&a->id),
            (void*) a);

    adbus_buf_append(a->buf, data, sz);

    while (!ret) {
        size_t len;
        const char* line = adbus_buf_line(a->buf, &len);

        /* Need more data */
        if (!line) {
            return sz;
        }

        ret = ProcessLine(a, line, len);
        adbus_buf_remove(a->buf, 0, len);
    }

    if (ret < 0) {
        /* Error */
        return -1;

    } else {
        /* Success */
        a->successful = 1;
        *finished = 1;
        return sz - adbus_buf_size(a->buf);
    }
}


