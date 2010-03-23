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

#pragma once

#include "internal.h"

ADBUSI_DATA adbus_LogCallback sLogFunction;

ADBUSI_FUNC int adbusI_logmsg(const char* header, const adbus_Message* msg);
ADBUSI_FUNC int adbusI_logbind(const char* header, const adbus_Bind* bind);
ADBUSI_FUNC int adbusI_logmatch(const char* header, const adbus_Match* match);
ADBUSI_FUNC int adbusI_logreply(const char* header, const adbus_Reply* reply);
ADBUSI_FUNC int adbusI_log(const char* format, ...);

#define ADBUSI_LOG_MSG(header, msg)     (sLogFunction && adbusI_logmsg(header, msg))
#define ADBUSI_LOG_BIND(header, bind)   (sLogFunction && adbusI_logbind(header, bind))
#define ADBUSI_LOG_MATCH(header, match) (sLogFunction && adbusI_logmatch(header, match))
#define ADBUSI_LOG_REPLY(header, reply) (sLogFunction && adbusI_logreply(header, reply))
#define ADBUSI_LOG                      sLogFunction && adbusI_log
