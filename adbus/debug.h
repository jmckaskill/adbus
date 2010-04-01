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

/* Log levels:
 * 1. Low - Low rate application debugging 
 *      - Binds
 *      - Matches
 *      - Connection create/free
 *      - Interface create/free
 *      - State create/free
 *      - Proxy create/free
 *
 * 2. Medium - High rate application debugging
 *      - Replies
 *      - State create/free
 *      - Message send, receive, and dispatch
 *      - Callback dispatch
 *
 * 3. High - Internal debugging
 *      - Message parsing
 *
 */

#ifdef __GNUC__
#   define ADBUSI_PRINTF(FORMAT, VARGS) __attribute__ ((format (printf, FORMAT, VARGS)))
#else
#   define ADBUSI_PRINTF(FORMAT, VARGS)
#endif

ADBUSI_DATA int adbusI_loglevel;

ADBUSI_FUNC void adbusI_initlog();
ADBUSI_FUNC void adbusI_logmsg(const adbus_Message* msg, const char* format, ...) ADBUSI_PRINTF(2,3);
ADBUSI_FUNC void adbusI_logbind(const adbus_Bind* bind, const char* format, ...) ADBUSI_PRINTF(2,3);
ADBUSI_FUNC void adbusI_logmatch(const adbus_Match* match, const char* format, ...) ADBUSI_PRINTF(2,3);
ADBUSI_FUNC void adbusI_logreply(const adbus_Reply* reply, const char* format, ...) ADBUSI_PRINTF(2,3);
ADBUSI_FUNC void adbusI_logdata(const char* buf, int sz, const char* format, ...) ADBUSI_PRINTF(3,4);
ADBUSI_FUNC void adbusI_log(const char* format, ...) ADBUSI_PRINTF(1,2);

#define ADBUSI_LOG_MSG_1    if (adbusI_loglevel < 1) {} else adbusI_logmsg
#define ADBUSI_LOG_MSG_2    if (adbusI_loglevel < 2) {} else adbusI_logmsg
#define ADBUSI_LOG_BIND_1   if (adbusI_loglevel < 1) {} else adbusI_logbind
#define ADBUSI_LOG_MATCH_1  if (adbusI_loglevel < 1) {} else adbusI_logmatch
#define ADBUSI_LOG_REPLY_1  if (adbusI_loglevel < 1) {} else adbusI_logreply
#define ADBUSI_LOG_REPLY_2  if (adbusI_loglevel < 2) {} else adbusI_logreply
#define ADBUSI_LOG_DATA_3   if (adbusI_loglevel < 3) {} else adbusI_logdata
#define ADBUSI_LOG_1        if (adbusI_loglevel < 1) {} else adbusI_log
#define ADBUSI_LOG_2        if (adbusI_loglevel < 2) {} else adbusI_log
#define ADBUSI_LOG_3        if (adbusI_loglevel < 3) {} else adbusI_log
