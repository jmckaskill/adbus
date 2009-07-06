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

#include "User.h"

#include <stdlib.h>

// ----------------------------------------------------------------------------

void ADBusUserClone(const struct ADBusUser* from, struct ADBusUser* to)
{
  if (to->free || to->data)
    ADBusUserFree(to);

  if (from == NULL) {
    ADBusUserInit(to);
    return;
  }

  if (from->clone)
    from->clone(from, to);
  else
    ADBusUserCloneDefault(from, to);
}

// ----------------------------------------------------------------------------

void ADBusUserFree(struct ADBusUser* data)
{
  if (data->free)
    data->free(data);
  else
    ADBusUserFreeDefault(data);
}

// ----------------------------------------------------------------------------

void ADBusUserCloneDefault(const struct ADBusUser* from, struct ADBusUser* to)
{
  if (from->size == 0) {
    memcpy(to, from, sizeof(struct ADBusUser));
  } else {
    memcpy(to, from, sizeof(struct ADBusUser));
    to->data = malloc(from->size);
    memcpy(to->data, from->data, from->size);
  }
}

// ----------------------------------------------------------------------------

void ADBusUserFreeDefault(struct ADBusUser* data)
{
  if (data->size)
    free(data->data);
}

// ----------------------------------------------------------------------------


