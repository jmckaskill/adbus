#include "Misc_p.h"


//-----------------------------------------------------------------------------

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
  /*110 n */ 2, 4, 0, 2, 0,    4, 8, 4, 4, 0,
  /*120 x */ 8, 1, 0, 8, 0,    0, 0, 0
};


int _DBusRequiredAlignment(char ch)
{
  ASSERT(ch < 128 && sRequiredAlignment[ch] > 0); 
  return sRequiredAlignment[ch & 0x80];
}

//-----------------------------------------------------------------------------

static const uint32_t kLittleEndianTestData = 1;
static const bool kLittleEndian = *(uint8_t*)(&kLittleEndianTestData) == 1;

const char _DBusNativeEndianness = kLittleEndian ? 'l' : 'B';

//-----------------------------------------------------------------------------

bool _DBusIsValidObjectPath(const char* str, size_t len)
{
  if (len == 0)
    return false;
  if (str[0] != '/')
    return false;
  if (len > 1 && str[len - 1] == '/')
    return false;

  // Now we check for consecutive '/' and that
  // the path components only use [A-Z][a-z][0-9]_
  const char* slash = str;
  const char* cur = str;
  while (++cur < str + len)
  {
    if (*cur == '/')
    {
      if (cur - slash == 1)
        return false;
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
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------

bool _DBusIsValidInterfaceName(const char* str, size_t len)
{
  if (len == 0 || len > 255)
    return false;

  // Must not begin with a digit
  if (!(  ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')))
  {
    return false;
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
        return false;
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
      return false;
    }
  }
  // Interface names must include at least one '.'
  if (dot == str - 1)
    return false;

  return true;
}

//-----------------------------------------------------------------------------

bool _DBusIsValidBusName(const char* str, size_t len)
{
  if (len == 0 || len > 255)
    return false;

  // Bus name must either begin with : or not a digit
  if (!(  (str[0] == ':')
       || ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')
       || (str[0] == '-')))
  {
    return false;
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
        return false;
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
      return false;
    }
  }
  // Bus names must include at least one '.'
  if (dot == str - 1)
    return false;

  return true;
}

//-----------------------------------------------------------------------------

bool _DBusIsValidMemberName(const char* str, size_t len)
{
  if (len == 0 || len > 255)
    return false;

  // Must not begin with a digit
  if (!(  ('a' <= str[0] && str[0] <= 'z')
       || ('A' <= str[0] && str[0] <= 'Z')
       || (str[0] == '_')))
  {
    return false;
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
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------

bool _DBusHasNullByte(const char* str, size_t len)
{
  return memchr(str, '\0', len) != NULL;
}

//-----------------------------------------------------------------------------

bool _DBusIsValidUtf8(const char* str, size_t len)
{
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
      return false;
    }
    else if (s[0] < 0xE0)
    {
      // 2 byte sequence
      if (end - s < 2)
        return false;

      // Check the continuation bytes
      if (s[1] < 0x80)
        return false;

      // Check for overlong encoding
      // 0x80 -> 0xC2 0x80
      if (s[0] < 0xC2)
        return false;

      s += 2;
    }
    else if (s[0] < 0xF0)
    {
      // 3 byte sequence
      if (end - s < 3)
        return false;

      // Check the continuation bytes
      if (s[1] < 0x80 || s[2] < 0x80)
        return false;

      // Check for overlong encoding
      // 0x08 00 -> 0xE0 A0 90
      if (s[0] == 0xE0 && s[1] < 0xA0)
        return false;

      // Code points [0xD800, 0xE000) are invalid
      // (UTF16 surrogate characters)
      // 0xD8 00 -> 0xED A0 80
      // 0xDF FF -> 0xED BF BF
      // 0xE0 00 -> 0xEE 80 80
      if (s[0] == 0xED && s[1] >= 0xA0)
        return false;

      s += 3;
    }
    else if (s[0] < 0xF5)
    {
      // 4 byte sequence
      if (end - s < 4)
        return false;

      // Check the continuation bytes
      if (s[1] < 0x80 || s[2] < 0x80 || s[3] < 0x80)
        return false;

      // Check for overlong encoding
      // 0x01 00 00 -> 0xF0 90 80 80
      if (s[0] == 0xF0 && s[1] < 0x90)
        return false;

      s += 4;
    }
    else
    {
      // 4 byte above 0x10FFFF, 5 byte, and 6 byte sequences
      // restricted by RFC 3639
      // 0xFE-0xFF invalid
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------



