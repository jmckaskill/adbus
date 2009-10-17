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
#include "str.h"

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

static void LocalId(str_t* id)
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
    str_clear(id);
    for (size_t i = 0; i < sidlen; ++i)
        str_append_char(id, (char) stringsid[i]);
#else
    str_set_n(id, stringsid);
#endif

    LocalFree(stringsid);

failed:
    if (process_token != NULL)
        CloseHandle(process_token);
}

#else

#include <unistd.h>
#include <sys/types.h>

static void LocalId(str_t* id)
{
    uid_t uid = geteuid();
    char buf[32];
    snprintf(buf, 31, "%d", (int) uid);
    buf[31] = '\0';
    str_set(id, buf);
}

#endif

// ----------------------------------------------------------------------------

static void HexDecode(str_t* out, const char* str, size_t size)
{
    if (size % 2 != 0)
        return;

    for (size_t i = 0; i < size / 2; ++i)
    {
        char hi = str[2*i];
        if ('0' <= hi && hi <= '9')
            hi = hi - '0';
        else if ('a' <= hi && hi <= 'f')
            hi = hi - 'a' + 10;
        else if ('A' <= hi && hi <= 'F')
            hi = hi - 'A' + 10;
        else
            return;

        char lo = str[2*i + 1];
        if ('0' <= lo && lo <= '9')
            lo = lo - '0';
        else if ('a' <= lo && lo <= 'f')
            lo = lo - 'a' + 10;
        else if ('A' <= lo && lo <= 'F')
            lo = lo - 'A' + 10;
        else
            return;

        str_append_char(out, (hi << 4) | lo);
    }
}

// ----------------------------------------------------------------------------

static void HexEncode(str_t* out, const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        char hi = data[i] >> 4;
        if (hi < 10)
            hi += '0';
        else
            hi += 'a' - 10;

        char lo = data[i] & 0x0F;
        if (lo < 10)
            lo += '0';
        else
            lo += 'a' - 10;

        str_append_char(out, hi);
        str_append_char(out, lo);
    }
}

// ----------------------------------------------------------------------------

static void GetCookie(const char* keyring,
                      const char* id,
                      str_t* cookie)
{
    str_t keyringFile = NULL;
#ifdef WIN32
    const char* home = getenv("userprofile");
#else
    const char* home = getenv("HOME");
#endif
    if (home) {
        str_append(&keyringFile, home);
        str_append(&keyringFile, "/");
    }
    // If no HOME env then we go off the current directory
    str_append(&keyringFile, ".dbus-keyrings/");
    str_append(&keyringFile, keyring);
    FILE* file = fopen(keyringFile, "r");
    str_free(&keyringFile);
    if (!file)
        return;
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
            str_set_n(cookie, dataBegin, dataEnd - dataBegin);
            break;
        }
    }
    fclose(file);
}

// ----------------------------------------------------------------------------

static int ParseServerData(const char* data,
                           str_t* keyring,
                           str_t* id,
                           str_t* serverData)
{
    str_t decoded = NULL;
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
    
    HexDecode(&decoded, hexDataBegin, hexDataEnd - hexDataBegin);
    
    keyringEnd = strchr(decoded, ' ');
    if (!keyringEnd)
        goto err;

    if (keyring)    
        str_set_n(keyring, decoded, keyringEnd - decoded);
    
    idBegin = keyringEnd + 1;
    idEnd = strchr(idBegin, ' ');
    if (!idEnd)
        goto err;
    
    if (id)
        str_set_n(id, idBegin, idEnd - idBegin);
    if (serverData)
        str_set(serverData, idEnd + 1);

    str_free(&decoded);
    return 0;

err:
    str_free(&decoded);
    return 1;
}

// ----------------------------------------------------------------------------

static void GenerateReply(const char*   hexserver,
                          const char*   hexcookie,
                          uint8_t*      localData,
                          size_t        localDataSize,
                          str_t* reply)
{
    str_t shastr = NULL;
    str_t replyarg = NULL;

    str_set(&shastr, hexserver);
    str_append(&shastr, ":");
    HexEncode(&shastr, localData, localDataSize);
    str_append(&shastr, ":");
    str_append(&shastr, hexcookie);

    SHA1 sha;
    SHA1Init(&sha);
    SHA1AddBytes(&sha, shastr, str_size(&shastr));

    str_clear(&replyarg);
    HexEncode(&replyarg, localData, localDataSize);
    str_append(&replyarg, " ");
    HexEncode(&replyarg, SHA1GetDigest(&sha), 20);

    str_set(reply, "DATA ");
    HexEncode(reply, (const uint8_t*) replyarg, str_size(&replyarg));
    str_append(reply, "\r\n");

    str_free(&shastr);
    str_free(&replyarg);
}

// ----------------------------------------------------------------------------

void ADBusAuthDBusCookieSha1(
        ADBusAuthSendCallback       send,
        ADBusAuthRecvCallback       recv,
        ADBusAuthRandCallback       rand,
        void*                       data)
{
    str_t id         = NULL;
    str_t auth       = NULL;
    str_t cookie     = NULL;
    str_t keyring    = NULL;
    str_t keyringid  = NULL;
    str_t servdata   = NULL;
    str_t reply      = NULL;

    send(data, "\0", 1);

    LocalId(&id);

    str_append(&auth, "AUTH DBUS_COOKIE_SHA1 ");
    HexEncode(&auth, (const uint8_t*) id, str_size(&id));
    str_append(&auth, "\r\n");

    send(data, auth, str_size(&auth));

    char buf[4096];
    int len = recv(data, buf, 4096);
    buf[len] = '\0';

    uint8_t randomData[32];
    for (size_t i = 0; i < 32; ++i)
        randomData[i] = rand(data);

    ParseServerData(buf, &keyring, &keyringid, &servdata);
    GetCookie(keyring, keyringid, &cookie);
    GenerateReply(servdata, cookie, randomData, 32, &reply);

    send(data, reply, str_size(&reply));
    len = recv(data, buf, 4096);
    send(data, "BEGIN\r\n", strlen("BEGIN\r\n"));

    str_free(&id);
    str_free(&auth);
    str_free(&cookie);
    str_free(&keyring);
    str_free(&keyringid);
    str_free(&servdata);
    str_free(&reply);
}

// ----------------------------------------------------------------------------

void ADBusAuthExternal(
        ADBusAuthSendCallback       send,
        ADBusAuthRecvCallback       recv,
        void*                       data)
{
    str_t id = NULL;
    str_t auth = NULL;

    send(data, "\0", 1);

    LocalId(&id);

    str_append(&auth, "AUTH EXTERNAL ");
    HexEncode(&auth, (const uint8_t*) id, str_size(&id));
    str_append(&auth, "\r\n");

    send(data, auth, str_size(&auth));

    char buf[4096];
    int len = recv(data, buf, 4096);
    buf[len] = '\0';

    send(data, "BEGIN\r\n", strlen("BEGIN\r\n"));

    str_free(&id);
    str_free(&auth);
}










