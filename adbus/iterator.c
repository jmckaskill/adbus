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

#include "internal.h"
#include "messages.h"

/** Skip over a single complete type.
 * \relates adbus_Iterator
 */
int adbus_iter_value(adbus_Iterator* i)
{
    switch (*i->sig)
    {
        case 'b': /* bool */
            return adbus_iter_bool(i, NULL);

        case 'y': /* u8 */
            return adbus_iter_u8(i, NULL);

        case 'n': /* i16 */
            return adbus_iter_i16(i, NULL);
        case 'q': /* u16 */
            return adbus_iter_u16(i, NULL);

        case 'i': /* i32 */
            return adbus_iter_i32(i, NULL);
        case 'u': /* u32 */
            return adbus_iter_u32(i, NULL);

        case 'x': /* i64 */
            return adbus_iter_i64(i, NULL);
        case 't': /* u64 */
            return adbus_iter_u64(i, NULL);

        case 'd': /* double */
            return adbus_iter_double(i, NULL);

        case 's': /* string */
            return adbus_iter_string(i, NULL, NULL);

        case 'o': /* object path */
            return adbus_iter_objectpath(i, NULL, NULL);

        case 'g': /* signature */
            return adbus_iter_signature(i, NULL, NULL);

        case 'v': /* variant */
            {
                adbus_IterVariant v;
                return adbus_iter_beginvariant(i, &v)
                    || adbus_iter_value(i)
                    || adbus_iter_endvariant(i, &v);
            }

        case 'a': /* array */
            {
                adbus_IterArray a;
                return adbus_iter_beginarray(i, &a)
                    || adbus_iter_endarray(i, &a);
            }

        case '(': /* struct */
            if (adbus_iter_beginstruct(i))
                return -1;
            while (*i->sig != ')') {
                if (adbus_iter_value(i)) {
                      return -1;
                }
            }
            return adbus_iter_endstruct(i);

        case '{': /* dict entry */
            if (adbus_iter_begindictentry(i))
                return -1;
            if (adbus_iter_value(i))
                return -1;
            if (adbus_iter_value(i))
                return -1;
            return adbus_iter_enddictentry(i);

        default:
            assert(0);
            return -1;
    }
}

/** Setup an iterator for the data in an adbus_Buffer.
 * \relates adbus_Iterator
 */
void adbus_iter_buffer(adbus_Iterator* i, const adbus_Buffer* buf)
{
    i->data = adbus_buf_data(buf);
    i->end  = i->data + adbus_buf_size(buf);
    i->sig  = adbus_buf_sig(buf, NULL);
}

/** Setup an iterator for the arguments in an adbus_Message.
 * \relates adbus_Iterator
 */
void adbus_iter_args(adbus_Iterator* i, const adbus_Message* msg)
{
    i->data = msg->argdata;
    i->end  = msg->argdata + msg->argsize;
    i->sig  = msg->signature;
}



static int Flip16(adbus_Iterator* i)
{
    uint16_t* p;

    p       = (uint16_t*) ADBUS_ALIGN(i->data, 2);
    i->data = (char*) (p + 1);

    if (i->data > i->end)
        return -1;

    *p =  ((*p & UINT16_C(0xFF00)) >> 8)
        | ((*p & UINT16_C(0x00FF)) << 8);

    return 0;
}

static int Flip32(adbus_Iterator* i, adbus_Bool consume)
{
    uint32_t* p;

    p       = (uint32_t*) ADBUS_ALIGN(i->data, 4);
    i->data = (char*) (p + 1);

    if (i->data > i->end)
        return -1;

    *p =  ((*p & UINT32_C(0xFF000000)) >> 24)
        | ((*p & UINT32_C(0x00FF0000)) >> 8)
        | ((*p & UINT32_C(0x0000FF00)) << 8)
        | ((*p & UINT32_C(0x000000FF)) << 24);

    if (!consume) {
        i->data -= 4;
    }

    return 0;
}

static int Flip64(adbus_Iterator* i)
{
    uint64_t* p;

    p       = (uint64_t*) ADBUS_ALIGN(i->data, 8);
    i->data = (char*) (p + 1);

    if (i->data > i->end)
        return -1;

    *p =  ((*p & UINT64_C(0xFF00000000000000)) >> 56)
        | ((*p & UINT64_C(0x00FF000000000000)) >> 40)
        | ((*p & UINT64_C(0x0000FF0000000000)) >> 24)
        | ((*p & UINT64_C(0x000000FF00000000)) >> 8)
        | ((*p & UINT64_C(0x00000000FF000000)) << 8)
        | ((*p & UINT64_C(0x0000000000FF0000)) << 16)
        | ((*p & UINT64_C(0x000000000000FF00)) << 40)
        | ((*p & UINT64_C(0x00000000000000FF)) << 56);

    return 0;
}

static int FlipArray(adbus_Iterator* i)
{
    adbus_IterArray a;
    if (Flip32(i, 0) || adbus_iter_beginarray(i, &a))
        return -1;

    while (adbus_iter_inarray(i, &a)) {
        if (adbus_flip_value((char**) &i->data, i->end, &i->sig)) {
            return -1;
        }
    }

    return adbus_iter_endarray(i, &a);
}

static int FlipVariant(adbus_Iterator* i)
{
    adbus_IterVariant v;
    if (adbus_iter_beginvariant(i, &v))
        return -1;

    adbus_flip_value((char**) &i->data, i->end, &i->sig);

    return adbus_iter_endvariant(i, &v);
}


/** Endian flips a single complete type.
 * \relates adbus_Buffer
 */
int adbus_flip_value(char** data, const char* end, const char** sig)
{
    int ret = 0;
    adbus_Iterator i;
    
    i.data = *data;
    i.end  = end;
    i.sig  = *sig;

    switch (*(i.sig)++) 
    {
        case 'y':
            ret = adbus_iter_u8(&i, NULL);
            break;

        case 'n':
        case 'q':
            ret = Flip16(&i);
            break;

        case 'b':
        case 'i':
        case 'u':
            ret = Flip32(&i, 1);
            break;

        case 'x':
        case 't':
            ret = Flip64(&i);
            break;

        case 's':
            ret = Flip32(&i, 0)
                || adbus_iter_string(&i, NULL, NULL);
            break;

        case 'o':
            ret = Flip32(&i, 0)
                || adbus_iter_objectpath(&i, NULL, NULL);
            break;

        case 'g':
            ret = adbus_iter_signature(&i, NULL, NULL);
            break;

        case 'v':
            ret = FlipVariant(&i);
            break;

        case 'a':
            ret = FlipArray(&i);
            break;

        case '(':
            ret = adbus_iter_beginstruct(&i);
            while (!ret && *i.sig && *i.sig != ')') {
                ret = adbus_flip_value((char**) &i.data, i.end, &i.sig);
            }
            ret = ret || adbus_iter_endstruct(&i);
            break;

        case '{':
            ret = adbus_iter_begindictentry(&i);
            ret = ret || adbus_flip_value((char**) &i.data, i.end, &i.sig);
            ret = ret || adbus_flip_value((char**) &i.data, i.end, &i.sig);
            ret = ret || adbus_iter_enddictentry(&i);
            break;

        default:
            assert(0);
            ret = -1;
            break;
    }

    *data = (char*) i.data;
    *sig  = i.sig;
    return ret;
}

/** Endian flips the entire buffer.
 * \relates adbus_Buffer
 */
int adbus_flip_data(char* data, const char* end, const char* sig)
{
    while (data < end) {
        if (adbus_flip_value(&data, end, &sig)) {
            return -1;
        }
    }
    return 0;
}




/* -------------------------------------------------------------------------
 * Check functions
 * -------------------------------------------------------------------------
 */

static void Sig(adbus_CbData* d, char field)
{
    if (d->checkiter.sig == NULL || *d->checkiter.sig != field) {
        adbus_error_argument(d);
        longjmp(d->jmpbuf, ADBUSI_ERROR);
    }
}

static void Iter(adbus_CbData* d, int ret)
{
    if (ret) {
        longjmp(d->jmpbuf, ADBUSI_PARSE_ERROR);
    }
}

/** Check that all arguments have been consumed.
 *  \relates adbus_CbData
 */
void adbus_check_end(adbus_CbData* d)
{
    if (d->checkiter.sig && *d->checkiter.sig) {
        adbus_error_argument(d);
        longjmp(d->jmpbuf, ADBUSI_ERROR);
    }
}

/** Pull out a boolean
 *  \relates adbus_CbData
 */
adbus_Bool adbus_check_bool(adbus_CbData* d)
{
    adbus_Bool ret;
    Sig(d, 'b');
    Iter(d, adbus_iter_bool(&d->checkiter, &ret));
    return ret;
}

/** Pull out a uint8
 *  \relates adbus_CbData
 */
uint8_t adbus_check_u8(adbus_CbData* d)
{
    uint8_t ret;
    Sig(d, 'y');
    Iter(d, adbus_iter_u8(&d->checkiter, &ret));
    return ret;
}

/** Pull out a int16
 *  \relates adbus_CbData
 */
int16_t adbus_check_i16(adbus_CbData* d)
{
    int16_t ret;
    Sig(d, 'n');
    Iter(d, adbus_iter_i16(&d->checkiter, &ret));
    return ret;
}

/** Pull out a uint16
 *  \relates adbus_CbData
 */
uint16_t adbus_check_u16(adbus_CbData* d)
{
    uint16_t ret;
    Sig(d, 'q');
    Iter(d, adbus_iter_u16(&d->checkiter, &ret));
    return ret;
}

/** Pull out a int32
 *  \relates adbus_CbData
 */
int32_t adbus_check_i32(adbus_CbData* d)
{
    int32_t ret;
    Sig(d, 'i');
    Iter(d, adbus_iter_i32(&d->checkiter, &ret));
    return ret;
}

/** Pull out a uint32
 *  \relates adbus_CbData
 */
uint32_t adbus_check_u32(adbus_CbData* d)
{
    uint32_t ret;
    Sig(d, 'u');
    Iter(d, adbus_iter_u32(&d->checkiter, &ret));
    return ret;
}

/** Pull out a int64
 *  \relates adbus_CbData
 */
int64_t adbus_check_i64(adbus_CbData* d)
{
    int64_t ret;
    Sig(d, 'x');
    Iter(d, adbus_iter_i64(&d->checkiter, &ret));
    return ret;
}

/** Pull out a uint64
 *  \relates adbus_CbData
 */
uint64_t adbus_check_u64(adbus_CbData* d)
{
    uint64_t ret;
    Sig(d, 't');
    Iter(d, adbus_iter_u64(&d->checkiter, &ret));
    return ret;
}

/** Pull out a double
 *  \relates adbus_CbData
 */
double adbus_check_double(adbus_CbData* d)
{
    double ret;
    Sig(d, 'd');
    Iter(d, adbus_iter_double(&d->checkiter, &ret));
    return ret;
}

/** Pull out a string
 *  \relates adbus_CbData
 */
const char* adbus_check_string(adbus_CbData* d, size_t* size)
{
    const char* ret;
    Sig(d, 's');
    Iter(d, adbus_iter_string(&d->checkiter, &ret, size));
    return ret;
}

/** Pull out a object path
 *  \relates adbus_CbData
 */
const char* adbus_check_objectpath(adbus_CbData* d, size_t* size)
{
    const char* ret;
    Sig(d, 's');
    Iter(d, adbus_iter_string(&d->checkiter, &ret, size));
    return ret;
}

/** Pull out a signature
 *  \relates adbus_CbData
 */
const char* adbus_check_signature(adbus_CbData* d, size_t* size)
{
    const char* ret;
    Sig(d, 's');
    Iter(d, adbus_iter_string(&d->checkiter, &ret, size));
    return ret;
}

/** Begin pulling out items in an array
 *  \relates adbus_CbData
 */
void adbus_check_beginarray(adbus_CbData* d, adbus_IterArray* a)
{
    Sig(d, 'a');
    Iter(d, adbus_iter_beginarray(&d->checkiter, a));
}

/** Check if there are more entries to pull out of the array
 *  \relates adbus_CbData
 */
adbus_Bool adbus_check_inarray(adbus_CbData* d, adbus_IterArray* a)
{ return adbus_iter_inarray(&d->checkiter, a); }

/** Finish pulling out items in the array
 *  \relates adbus_CbData
 */
void adbus_check_endarray(adbus_CbData* d, adbus_IterArray* a)
{ Iter(d, adbus_iter_endarray(&d->checkiter, a)); }

/** Begin pulling out items in a struct
 *  \relates adbus_CbData
 */
void adbus_check_beginstruct(adbus_CbData* d)
{
    Sig(d, '(');
    Iter(d, adbus_iter_beginstruct(&d->checkiter));
}

/** Finish pulling out items in a struct
 *  \relates adbus_CbData
 */
void adbus_check_endstruct(adbus_CbData* d)
{
    Sig(d, ')');
    Iter(d, adbus_iter_endstruct(&d->checkiter));
}

/** Begin pulling out items in a dict entry
 *  \relates adbus_CbData
 */
void adbus_check_begindictentry(adbus_CbData* d)
{
    Sig(d, '{');
    Iter(d, adbus_iter_begindictentry(&d->checkiter));
}

/** Finish pulling out items in a dict entry
 *  \relates adbus_CbData
 */
void adbus_check_enddictentry(adbus_CbData* d)
{
    Sig(d, '}');
    Iter(d, adbus_iter_enddictentry(&d->checkiter));
}

/** Begin pulling out data in a variant
 *  \relates adbus_CbData
 */
const char* adbus_check_beginvariant(adbus_CbData* d, adbus_IterVariant* v)
{
    Sig(d, 'v');
    Iter(d, adbus_iter_beginvariant(&d->checkiter, v));
    return d->checkiter.sig;
}

/** Finish pulling out data in a variant
 *  \relates adbus_CbData
 */
void adbus_check_endvariant(adbus_CbData* d, adbus_IterVariant* v)
{
    Sig(d, '\0');
    Iter(d, adbus_iter_endvariant(&d->checkiter, v));
}

/** Iterate over a complete argument
 *  \relates adbus_CbData
 */
void adbus_check_value(adbus_CbData* d)
{
    if (!d->checkiter.sig || !*d->checkiter.sig) {
        adbus_error_argument(d);
        longjmp(d->jmpbuf, ADBUSI_ERROR);
    }

    Iter(d, adbus_iter_value(&d->checkiter));
}

