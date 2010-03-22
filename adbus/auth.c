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

#define ADBUS_LIBRARY
#include "misc.h"
#include "sha1.h"

#include "dmem/string.h"
#include "dmem/queue.h"

#include <adbus.h>
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

DQUEUE_INIT(char, char);

/* -------------------------------------------------------------------------- */

#ifdef _WIN32
#define WINVER 0x0500	// Allow use of features specific to Windows 2000 or later.
#define _WIN32_WINNT 0x0501	// Allow use of features specific to Windows XP or later.
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

    // it should fail as it has no buffer
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
    if (size % 2 != 0)
        return;

    const uint8_t* ustr = (const uint8_t*) str;
    for (size_t i = 0; i < size / 2; ++i)
    {
        uint8_t hi = ustr[2*i];
        if ('0' <= hi && hi <= '9')
            hi = hi - '0';
        else if ('a' <= hi && hi <= 'f')
            hi = hi - 'a' + 10;
        else if ('A' <= hi && hi <= 'F')
            hi = hi - 'A' + 10;
        else
            return;

        uint8_t lo = ustr[2*i + 1];
        if ('0' <= lo && lo <= '9')
            lo = lo - '0';
        else if ('a' <= lo && lo <= 'f')
            lo = lo - 'a' + 10;
        else if ('A' <= lo && lo <= 'F')
            lo = lo - 'A' + 10;
        else
            return;

        ds_cat_char(out, (hi << 4) | lo);
    }
}

/* -------------------------------------------------------------------------- */

static void HexEncode(d_String* out, const char* data, size_t size)
{
    const uint8_t* udata = (const uint8_t*) data;
    for (size_t i = 0; i < size; ++i)
    {
        uint8_t hi = udata[i] >> 4;
        if (hi < 10)
            hi += '0';
        else
            hi += 'a' - 10;

        uint8_t lo = udata[i] & 0x0F;
        if (lo < 10)
            lo += '0';
        else
            lo += 'a' - 10;

        ds_cat_char(out, hi);
        ds_cat_char(out, lo);
    }
}

/* -------------------------------------------------------------------------- */
static int GetCookie(const char* keyring,
                      const char* id,
                      d_String* cookie)
{
    d_String keyringFile;
    ZERO(&keyringFile);
#ifdef _WIN32
    const char* home = getenv("userprofile");
#else
    const char* home = getenv("HOME");
#endif
    if (home) {
        ds_cat(&keyringFile, home);
        ds_cat(&keyringFile, "/");
    }
    // If no HOME env then we go off the current directory
    ds_cat(&keyringFile, ".dbus-keyrings/");
    ds_cat(&keyringFile, keyring);
    FILE* file = fopen(ds_cstr(&keyringFile), "r");
    if (!file)
        goto end;
    char buf[4096];
    while(fgets(buf, 4096, file))
    {
        // Find the line that begins with id
        if (strncmp(buf, id, strlen(id)) == 0)
        {
            const char* bufEnd = buf + strlen(buf);
            const char* idEnd = buf + strlen(id);
            if (idEnd >= bufEnd)
                break;

            const char* timeBegin = idEnd + 1;
            const char* timeEnd = strchr(timeBegin, ' ');
            if (!timeEnd)
                break;

            const char* dataBegin = timeEnd + 1;
            const char* dataEnd = bufEnd - 1; // -1 for \n
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
    const char* keyringEnd = strchr(data, ' ');
    if (!keyringEnd)
        return 1;

    if (keyring)    
        ds_set_n(keyring, data, keyringEnd - data);
    
    const char* idBegin = keyringEnd + 1;
    const char* idEnd = strchr(idBegin, ' ');
    if (!idEnd)
        return 1;
    
    if (id)
        ds_set_n(id, idBegin, idEnd - idBegin);
    if (serverData)
        ds_set(serverData, idEnd + 1);

    return 0;
}







/* -------------------------------------------------------------------------- */
enum
{
    NOT_TRIED,
    NOT_SUPPORTED,
    BEGIN,
};

typedef int (*DataCallback)(adbus_Auth* a, d_String* data);

struct adbus_Auth
{
    /** \privatesection */
    adbus_Bool              server;
    int                     cookie;
    int                     external;
    adbus_SendCallback      send;
    adbus_RandCallback      rand;
    adbus_ExternalCallback  externalcb;
    void*                   data;
    DataCallback            onData;
    d_Queue(char)           buf;
    d_String                id;
    d_String                okCmd;
    adbus_Bool              okSent;
};

/* -------------------------------------------------------------------------- */
/** Free an auth structure 
 *  \relates adbus_Auth
 */
void adbus_auth_free(adbus_Auth* a)
{
    if (a) {
        dq_free(char, &a->buf);
        ds_free(&a->id);
        ds_free(&a->okCmd);
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
    a->server       = 1;
    a->external     = NOT_SUPPORTED;
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
ADBUS_API void adbus_sauth_external(
        adbus_Auth*             a,
        adbus_ExternalCallback  cb)
{
    assert(a->server && a->external == NOT_SUPPORTED);
    a->external   = NOT_TRIED;
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
    a->server       = 0;
    a->external     = NOT_SUPPORTED;
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
ADBUS_API void adbus_cauth_external(adbus_Auth* a)
{
    assert(!a->server && a->external == NOT_SUPPORTED);
    a->external   = NOT_TRIED;
}

/* -------------------------------------------------------------------------- */
static int ClientReset(adbus_Auth* a);

/** Initiates the client auth
 *  \relates adbus_Auth
 */
ADBUS_API int adbus_cauth_start(adbus_Auth* a)
{ return ClientReset(a); }





/* -------------------------------------------------------------------------- */
static void RandomData(adbus_Auth* a, d_String* str)
{
    for (size_t i = 0; i < 64; i++) {
        ds_cat_char(str, a->rand(a->data));
    }
}

/* -------------------------------------------------------------------------- */
static int Send(adbus_Auth* a, const char* data)
{
    size_t sz = strlen(data);
    adbus_ssize_t sent = a->send(a->data, data, sz);
    if (sent < 0) {
        return -1;
    } else if (sent != (adbus_ssize_t) sz) {
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
    a->external = BEGIN;
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
    if (a->external == NOT_TRIED && MATCH(mech, mechsz, "EXTERNAL")) {
        return ServerExternalInit(a);
    } else {
        return ServerReset(a);
    }
}





/* -------------------------------------------------------------------------- */
static int ClientCookie(adbus_Auth* a, d_String* data)
{
    int ret = 0;
    d_String keyring;   ZERO(&keyring);
    d_String keyringid; ZERO(&keyringid);
    d_String servdata;  ZERO(&servdata);
    d_String cookie;    ZERO(&cookie);
    d_String shastr;    ZERO(&shastr);
    d_String reply;     ZERO(&reply);
    d_String replyarg;  ZERO(&replyarg);
    d_String localdata; ZERO(&localdata);
    uint8_t* digest     = NULL;
    SHA1 sha;


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

    SHA1Init(&sha);
    SHA1AddBytes(&sha, ds_cstr(&shastr), ds_size(&shastr));
    digest = SHA1GetDigest(&sha);

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
    ZERO(&msg);

    if (a->external == NOT_TRIED) {
        ds_cat(&msg, "AUTH EXTERNAL ");
        a->onData = NULL;
        a->external = BEGIN;
    } else if (a->cookie == NOT_TRIED) {
        ds_cat(&msg, "AUTH DBUS_COOKIE_SHA1 ");
        a->onData = &ClientCookie;
        a->cookie = BEGIN;
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
/** Parses a line received from the remote.
 *  \relates adbus_Auth
 *  
 *  This is mainly used if you want to filter the received auth lines.
 *  Otherwise its easier to use adbus_auth_parse().
 *
 *  \return 1 on success
 *  \return 0 on continue
 *  \return -1 on error
 */
int adbus_auth_line(adbus_Auth* a, const char* line, size_t len)
{
    if (line[len - 1] == '\n')
        len--;
    if (line[len - 1] == '\r')
        len--;

    const char* end = line + len;

    const char* cmdb = line;
    const char* cmde = (const char*) memchr(cmdb, ' ', end - cmdb);
    if (cmde == NULL)
        cmde = end;
    size_t cmdsz = cmde - cmdb;

    if (a->server && cmde != end && MATCH(cmdb, cmdsz, "AUTH")) {
        const char* mechb = cmde + 1;
        const char* meche = (const char*) memchr(mechb, ' ', end - mechb);
        if (meche) {
            const char* datab = meche + 1;
            d_String data;
            ZERO(&data);

            HexDecode(&data, datab, end - datab);

            int ret = ServerAuth(a, mechb, meche - mechb, &data);
            ds_free(&data);
            return ret;

        } else {
            Send(a, "ERROR\r\n");
            return 0;
        }

    } else if (a->onData && MATCH(cmdb, cmdsz, "DATA")) {
        d_String data;
        ZERO(&data);
        if (cmde != end) {
            const char* datab = line + sizeof("DATA");
            HexDecode(&data, datab, end - datab);
        }

        int ret = a->onData(a, &data);
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
        // We have no way of handling these
        return -1;

    } else {
        Send(a, "ERROR\r\n");
        return 0;
    }
}

/* -------------------------------------------------------------------------- */
/** Consumes auth lines from the supplied buffer
 *  \relates adbus_Auth
 *
 *  \return 1 on success
 *  \return 0 on continue
 *  \return -1 on error
 */
int adbus_auth_parse(adbus_Auth* a, adbus_Buffer* buf)
{
    size_t len;
    int ret = 0;
    const char* line = adbus_buf_line(buf, &len);
    while (line && !ret) {
        ret = adbus_auth_line(a, line, len);
        adbus_buf_remove(buf, 0, len);
        line = adbus_buf_line(buf, &len);
    }
    return ret;
}


