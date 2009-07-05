// vim: ts=2 sw=2 sts=2 et
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

#include "Misc_p.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


// ----------------------------------------------------------------------------

void _ADBusPrintDebug(const char* format, ...)
{
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "[adbus] ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

// ----------------------------------------------------------------------------

static const char sRequiredAlignment[128] =
{
  /*  0 \0*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 10 \n*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 20 SP*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 30 RS*/ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 40 ( */ 8, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 50 2 */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 60 < */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 70 F */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 80 P */ 0, 0, 0, 0, 0,    0, 0, 0, 0, 0,
  /* 90 Z */ 0, 0, 0, 0, 0,    0, 0, 4, 4, 0,
  /*100 d */ 8, 0, 0, 1, 0,    4, 0, 0, 0, 0,
  /*110 n */ 2, 4, 0, 2, 0,    4, 8, 4, 1, 0,
  /*120 x */ 8, 1, 0, 8, 0,    0, 0, 0
};


int _ADBusRequiredAlignment(char ch)
{
  assert(ch < 128 && sRequiredAlignment[(int)ch] > 0);
  return sRequiredAlignment[ch & 0x7F];
}

// ----------------------------------------------------------------------------

#ifdef ADBUS_LITTLE_ENDIAN
const char _ADBusNativeEndianness = 'l';
#elif defined(ADBUS_BIG_ENDIAN)
const char _ADBusNativeEndianness = 'B';
#else
#error Please define ADBUS_LITTLE_ENDIAN or ADBUS_BIG_ENDIAN
#endif

// ----------------------------------------------------------------------------

unsigned int _ADBusIsValidObjectPath(const char* str, size_t len)
{
  if (!str || len == 0)
    return 0;
  if (str[0] != '/')
    return 0;
  if (len > 1 && str[len - 1] == '/')
    return 0;

  // Now we check for consecutive '/' and that
  // the path components only use [A-Z][a-z][0-9]_
  const char* slash = str;
  const char* cur = str;
  while (++cur < str + len)
  {
    if (*cur == '/')
    {
      if (cur - slash == 1)
        return 0;
      slash = cur;
      continue;
    }
    else if ( ('a' <= *cur && *cur <= 'z')
           || ('A' <= *cur && *cur <= 'Z')
           || ('0' <= *cur && *cur <= '9')
           || *cur == '_')
    {
      continue;
    }
    else
    {
      return 0;
    }
  }
  return 1;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsValidInterfaceName(const char* str, size_t len)
{
  if (!str || len == 0 || len > 255)
    return 0;

  // Must not begin with a digit
  if (!(  ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')))
  {
    return 0;
  }

  // Now we check for consecutive '.' and that
  // the components only use [A-Z][a-z][0-9]_
  const char* dot = str - 1;
  const char* cur = str;
  while (++cur < str + len)
  {
    if (*cur == '.')
    {
      if (cur - dot == 1)
        return 0;
      dot = cur;
      continue;
    }
    else if ( ('a' <= *cur && *cur <= 'z')
           || ('A' <= *cur && *cur <= 'Z')
           || ('0' <= *cur && *cur <= '9')
           || (*cur == '_'))
    {
      continue;
    }
    else
    {
      return 0;
    }
  }
  // Interface names must include at least one '.'
  if (dot == str - 1)
    return 0;

  return 1;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsValidBusName(const char* str, size_t len)
{
  if (!str || len == 0 || len > 255)
    return 0;

  // Bus name must either begin with : or not a digit
  if (!(  (str[0] == ':')
       || ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')
       || (str[0] == '-')))
  {
    return 0;
  }

  // Now we check for consecutive '.' and that
  // the components only use [A-Z][a-z][0-9]_
  const char* dot = str[0] == ':' ? str : str - 1;
  const char* cur = str;
  while (++cur < str + len)
  {
    if (*cur == '.')
    {
      if (cur - dot == 1)
        return 0;
      dot = cur;
      continue;
    }
    else if ( ('a' <= *cur && *cur <= 'z')
           || ('A' <= *cur && *cur <= 'Z')
           || ('0' <= *cur && *cur <= '9')
           || (*cur == '_')
           || (*cur == '-'))
    {
      continue;
    }
    else
    {
      return 0;
    }
  }
  // Bus names must include at least one '.'
  if (dot == str - 1)
    return 0;

  return 1;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsValidMemberName(const char* str, size_t len)
{
  if (!str || len == 0 || len > 255)
    return 0;

  // Must not begin with a digit
  if (!(  ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')))
  {
    return 0;
  }

  // We now check that we only use [A-Z][a-z][0-9]_
  const char* cur = str;
  while (++cur < str + len)
  {
    if ( ('a' <= *cur && *cur <= 'z')
      || ('A' <= *cur && *cur <= 'Z')
      || ('0' <= *cur && *cur <= '9')
      || (*cur == '_'))
    {
      continue;
    }
    else
    {
      return 0;
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusHasNullByte(const char* str, size_t len)
{
  return memchr(str, '\0', len) != NULL;
}

// ----------------------------------------------------------------------------

unsigned int _ADBusIsValidUtf8(const char* str, size_t len)
{
  if (!str)
    return 1;
  const char* s = str;
  const char* end = str + len;
  while (s < end)
  {
    if (s[0] < 0x80)
    {
      // 1 byte sequence (US-ASCII)
      s += 1;
    }
    else if (s[0] < 0xC0)
    {
      // Multi-byte data without start
      return 0;
    }
    else if (s[0] < 0xE0)
    {
      // 2 byte sequence
      if (end - s < 2)
        return 0;

      // Check the continuation bytes
      if (!(0x80 <= s[1] && s[1] < 0xC0))
        return 0;

      // Check for overlong encoding
      // 0x80 -> 0xC2 0x80
      if (s[0] < 0xC2)
        return 0;

      s += 2;
    }
    else if (s[0] < 0xF0)
    {
      // 3 byte sequence
      if (end - s < 3)
        return 0;

      // Check the continuation bytes
      if (!(  0x80 <= s[1] && s[1] < 0xC0
           && 0x80 <= s[2] && s[2] < 0xC0))
        return 0;

      // Check for overlong encoding
      // 0x08 00 -> 0xE0 A0 90
      if (s[0] == 0xE0 && s[1] < 0xA0)
        return 0;

      // Code points [0xD800, 0xE000) are invalid
      // (UTF16 surrogate characters)
      // 0xD8 00 -> 0xED A0 80
      // 0xDF FF -> 0xED BF BF
      // 0xE0 00 -> 0xEE 80 80
      if (s[0] == 0xED && s[1] >= 0xA0)
        return 0;

      s += 3;
    }
    else if (s[0] < 0xF5)
    {
      // 4 byte sequence
      if (end - s < 4)
        return 0;

      // Check the continuation bytes
      if (!(  0x80 <= s[1] && s[1] < 0xC0
           && 0x80 <= s[2] && s[2] < 0xC0
           && 0x80 <= s[3] && s[3] < 0xC0))
        return 0;

      // Check for overlong encoding
      // 0x01 00 00 -> 0xF0 90 80 80
      if (s[0] == 0xF0 && s[1] < 0x90)
        return 0;

      s += 4;
    }
    else
    {
      // 4 byte above 0x10FFFF, 5 byte, and 6 byte sequences
      // restricted by RFC 3639
      // 0xFE-0xFF invalid
      return 0;
    }
  }

  return s == end ? 1 : 0;
}

// ----------------------------------------------------------------------------



