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

#include "Auth.h"

#include <string.h>


using namespace DBus;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int DBus::HexDecode(const char* str, int size, std::vector<uint8_t>* out)
{
  if (size < 0)
    size = strlen(str);
  if (size % 2 != 0)
    return 1;

  if (out)
    out->resize(size / 2);
  for (int i = 0; i < size / 2; ++i)
  {
    char hi = str[2*i];
    if ('0' <= hi && hi <= '9')
      hi = hi - '0';
    else if ('a' <= hi && hi <= 'f')
      hi = hi - 'a' + 10;
    else if ('A' <= hi && hi <= 'F')
      hi = hi - 'A' + 10;
    else
      return 1;

    char lo = str[2*i + 1];
    if ('0' <= lo && lo <= '9')
      lo = lo - '0';
    else if ('a' <= lo && lo <= 'f')
      lo = lo - 'a' + 10;
    else if ('A' <= lo && lo <= 'F')
      lo = lo - 'A' + 10;
    else
      return 1;

    if (out)
      (*out)[i] = (hi << 4) | lo;
  }
  return 0;
}

// ----------------------------------------------------------------------------

void DBus::HexEncode(const uint8_t* data, size_t size, std::string* out)
{
  if (!out)
    return;

  out->resize(size * 2);
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

    (*out)[2*i] = hi;
    (*out)[2*i + 1] = lo;
  }
}

// ----------------------------------------------------------------------------
