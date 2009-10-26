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

#include "Auth.h"
#include "sha1.h"
#include "memory/kstring.h"

#include <string.h>

#include <stdlib.h>
#include <stdio.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#ifdef WIN32
#include <malloc.h>
#include <windows.h>
#include <sddl.h>

static void LocalId(kstring_t* id)
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
    ks_clear(id);
    for (size_t i = 0; i < sidlen; ++i)
        ks_cat_char(id, (char) stringsid[i]);
#else
    ks_set(id, stringsid);
#endif

    LocalFree(stringsid);

failed:
    if (process_token != NULL)
        CloseHandle(process_token);
}

#else

#include <unistd.h>
#include <sys/types.h>

static void LocalId(kstring_t* id)
{
    uid_t uid = geteuid();
    char buf[32];
    snprintf(buf, 31, "%d", (int) uid);
    buf[31] = '\0';
    ks_set(id, buf);
}

#endif

// ----------------------------------------------------------------------------

static void HexDecode(kstring_t* out, const char* str, size_t size)
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

        ks_cat_char(out, (hi << 4) | lo);
    }
}

// ----------------------------------------------------------------------------

static void HexEncode(kstring_t* out, const char* data, size_t size)
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

        ks_cat_char(out, hi);
        ks_cat_char(out, lo);
    }
}

// ----------------------------------------------------------------------------

static void GetCookie(const char* keyring,
                      const char* id,
                      kstring_t* cookie)
{
    kstring_t* keyringFile = ks_new();
#ifdef WIN32
    const char* home = getenv("userprofile");
#else
    const char* home = getenv("HOME");
#endif
    if (home) {
        ks_cat(keyringFile, home);
        ks_cat(keyringFile, "/");
    }
    // If no HOME env then we go off the current directory
    ks_cat(keyringFile, ".dbus-keyrings/");
    ks_cat(keyringFile, keyring);
    FILE* file = fopen(ks_cstr(keyringFile), "r");
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
            ks_set_n(cookie, dataBegin, dataEnd - dataBegin);
            break;
        }
    }

end:
    if (file)
        fclose(file);
    ks_free(keyringFile);
}

// ----------------------------------------------------------------------------

static int ParseServerData(const char* data,
                           kstring_t* keyring,
                           kstring_t* id,
                           kstring_t* serverData)
{
    kstring_t* decoded = ks_new();
    const char* commandEnd = NULL;;
    const char* hexDataBegin = NULL;
    const char* hexDataEnd = NULL;
    const char* keyringEnd = NULL;
    const char* idBegin = NULL;
    const char* idEnd = NULL;

    commandEnd = strchr(data, ' ');
    if (!commandEnd)
        goto err;
    hexDataBegin = commandEnd + 1;
    hexDataEnd = strchr(hexDataBegin, '\r');
    if (!hexDataEnd)
        goto err;
    
    HexDecode(decoded, hexDataBegin, hexDataEnd - hexDataBegin);

    const char* cdecoded = ks_cstr(decoded);
    
    keyringEnd = strchr(cdecoded, ' ');
    if (!keyringEnd)
        goto err;

    if (keyring)    
        ks_set_n(keyring, cdecoded, keyringEnd - cdecoded);
    
    idBegin = keyringEnd + 1;
    idEnd = strchr(idBegin, ' ');
    if (!idEnd)
        goto err;
    
    if (id)
        ks_set_n(id, idBegin, idEnd - idBegin);
    if (serverData)
        ks_set(serverData, idEnd + 1);

    ks_free(decoded);
    return 0;

err:
    ks_free(decoded);
    return 1;
}

// ----------------------------------------------------------------------------

static void GenerateReply(const char*   hexserver,
                          const char*   hexcookie,
                          const char*   localData,
                          size_t        localDataSize,
                          kstring_t*    reply)
{
    kstring_t* shastr = ks_new();
    kstring_t* replyarg = ks_new();

    ks_set(shastr, hexserver);
    ks_cat(shastr, ":");
    HexEncode(shastr, localData, localDataSize);
    ks_cat(shastr, ":");
    ks_cat(shastr, hexcookie);

    SHA1 sha;
    SHA1Init(&sha);
    SHA1AddBytes(&sha, ks_cstr(shastr), ks_size(shastr));

    char* digest = (char*) SHA1GetDigest(&sha);

    ks_clear(replyarg);
    HexEncode(replyarg, localData, localDataSize);
    ks_cat(replyarg, " ");
    HexEncode(replyarg, digest, 20);
    free(digest);

    ks_set(reply, "DATA ");
    HexEncode(reply, ks_cstr(replyarg), ks_size(replyarg));
    ks_cat(reply, "\r\n");

    ks_free(shastr);
    ks_free(replyarg);
}

// ----------------------------------------------------------------------------

int ADBusAuthDBusCookieSha1(
        ADBusAuthSendCallback       send,
        ADBusAuthRecvCallback       recv,
        ADBusAuthRandCallback       rand,
        void*                       data)
{
    kstring_t* id         = ks_new();
    kstring_t* auth       = ks_new();
    kstring_t* cookie     = ks_new();
    kstring_t* keyring    = ks_new();
    kstring_t* keyringid  = ks_new();
    kstring_t* servdata   = ks_new();
    kstring_t* reply      = ks_new();

    send(data, "\0", 1);

    LocalId(id);

    ks_cat(auth, "AUTH DBUS_COOKIE_SHA1 ");
    HexEncode(auth, ks_cstr(id), ks_size(id));
    ks_cat(auth, "\r\n");

    send(data, ks_cstr(auth), ks_size(auth));

    char buf[4096];
    int len = recv(data, buf, 4096);
    buf[len] = '\0';

    char randomData[32];
    for (size_t i = 0; i < 32; ++i)
        randomData[i] = rand(data);

    ParseServerData(buf, keyring, keyringid, servdata);
    GetCookie(ks_cstr(keyring), ks_cstr(keyringid), cookie);
    GenerateReply(ks_cstr(servdata), ks_cstr(cookie), randomData, 32, reply);

    send(data, ks_cstr(reply), ks_size(reply));
    len = recv(data, buf, 4096);
    buf[len] = '\0';
    if (strncmp(buf, "OK ", strlen("OK ")) != 0)
        return -1;

    send(data, "BEGIN\r\n", strlen("BEGIN\r\n"));

    ks_free(id);
    ks_free(auth);
    ks_free(cookie);
    ks_free(keyring);
    ks_free(keyringid);
    ks_free(servdata);
    ks_free(reply);
    return 0;
}

// ----------------------------------------------------------------------------

int ADBusAuthExternal(
        ADBusAuthSendCallback       send,
        ADBusAuthRecvCallback       recv,
        void*                       data)
{
    kstring_t* id = ks_new();
    kstring_t* auth = ks_new();

    send(data, "\0", 1);

    LocalId(id);

    ks_cat(auth, "AUTH EXTERNAL ");
    HexEncode(auth, ks_cstr(id), ks_size(id));
    ks_cat(auth, "\r\n");

    send(data, ks_cstr(auth), ks_size(auth));

    char buf[4096];
    int len = recv(data, buf, 4096);
    buf[len] = '\0';
    if (strncmp(buf, "OK ", strlen("OK ")) != 0)
        return -1;

    send(data, "BEGIN\r\n", strlen("BEGIN\r\n"));

    ks_free(id);
    ks_free(auth);
    return 0;
}










