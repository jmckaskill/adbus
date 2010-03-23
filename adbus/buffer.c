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
#include "dmem/vector.h"

/** \struct adbus_Buffer
 *
 *  \brief General purpose buffer and means to serialise dbus data with an
 *  associated signature
 *
 *  \section general Using as a General Buffer
 *
 *  When being used as a general buffer data can be appended by using
 *  adbus_buf_append() or adbus_buf_recvbuf(); removed by using
 *  adbus_buf_remove(), adbus_buf_release(), or adbus_buf_reset(); and parsed
 *  with adbus_buf_line().
 *
 *
 *
 *  \section serialising Serialising DBus Data
 *
 *  When being used for serialising dbus arguments, the signature must be set
 *  first with adbus_buf_setsig() or adbus_buf_appendsig() and then data can be
 *  serialised using the various adbus_buf_bool() etc functions.
 *
 *  The serialising functions all track the supplied signature and assert when
 *  the wrong serialise function is used. This tracked signature can then be
 *  pulled out via adbus_buf_signext(). This is useful when serialising
 *  variant or typeless data.
 *
 *  The dbus data types which are scoped (array, dict entries, structs and
 *  variants) have a begin and end function and if needed use a struct in the
 *  calling code to maintain their scoped data.
 *
 *
 *
 *  \subsection arrays Arrays
 *
 *  Array scopes are special in that multiple array entries can be added in an
 *  array. These entries can be added by either calling adbus_buf_arrayentry()
 *  before each entry or by appending the entire array data via
 *  adbus_buf_append(). 
 *
 *  \warning Array data appended using adbus_buf_append() must have the
 *  correct alignment and have zero initialised padding.
 *
 *  Using adbus_buf_arrayentry() to serialise "ai" (an array of int32s):
 *
 *  \code
 *  uint32_t data[] = {1, 2, 3};
 *  adbus_Array a;
 *  adbus_buf_reset(buf);
 *  adbus_buf_setsig(buf, "ai");
 *  adbus_buf_beginarray(buf, &a);
 *  for (int i = 0; i < 3; i++)
 *  {
 *      adbus_buf_arrayentry(buf, &a);
 *      adbus_buf_u32(buf, data[i]);
 *  }
 *  adbus_buf_endarray(buf, &a);
 *  \endcode
 *
 *  Or equivalently using adbus_buf_append():
 *
 *  \code
 *  uint32_t data[] = {1, 2, 3};
 *  adbus_Array a;
 *  adbus_buf_reset(buf);
 *  adbus_buf_setsig(buf, "ai");
 *  adbus_buf_beginarray(buf, &a);
 *  adbus_buf_append(buf, data, sizeof(data));
 *  adbus_buf_endarray(buf, &a);
 *  \endcode
 *
 *
 *
 *  \subsection dictentry Dict Entries
 *
 *  Dict Entries can only be used as a scope directly inside an array. Thus the
 *  signature will always look like a{...}. The adbus_buf_begindictentry() call
 *  should occur after the adbus_buf_arrayentry().
 *
 *  For example to serialise "a{is}" (a map of int32 to string):
 *
 *  \code
 *  typedef std::map<int, std::string> Map;
 *  Map map;
 *  adbus_Array a;
 *  adbus_buf_reset(buf);
 *  adbus_buf_setsig(buf, "a{is}");
 *  adbus_buf_beginarray(buf, &a);
 *  for (Map::const_iterator ii = map.begin(); ii != map.end(); ++ii)
 *  {
 *      adbus_buf_arrayentry(buf, &a);
 *      adbus_buf_begindictentry(buf);
 *      adbus_buf_i32(buf, ii->first);
 *      adbus_buf_string(buf, ii->second.c_str(), -1);
 *      adbus_buf_enddictentry(buf);
 *  }
 *  adbus_buf_endarray(buf, &a);
 *  \endcode
 *
 *
 *
 *  \subsection struct Structs
 *
 *  Structs are fairly straight forward. They simply have a begin and end
 *  function with no scoped data.
 *
 *  For example to serialise "(iis)" (a struct containing 2 int32s and a
 *  string):
 *
 *  \code
 *  adbus_buf_reset(buf);
 *  adbus_buf_setsig(buf, "(iis)");
 *  adbus_buf_beginstruct(buf);
 *  adbus_buf_i32(buf, first_number);
 *  adbus_buf_i32(buf, second_number);
 *  adbus_buf_string(buf, string, -1);
 *  adbus_buf_endstruct(buf);
 *  \endcode
 *
 *
 *
 *  \subsection variant Variants
 *
 *  Variants have a scoped struct to hold the original signature value and a
 *  begin function which takes the signature of the variant data.
 *
 *  For example to serialise "v" with a variant signature of "s" (string):
 *
 *  \code
 *  adbus_Variant v;
 *  adbus_buf_reset(buf);
 *  adbus_buf_setsig(buf, "v");
 *  adbus_buf_beginvariant(buf, &v, "s", -1);
 *  adbus_buf_string(buf, string, -1);
 *  adbus_buf_endvariant(buf, &v);
 *  \endcode
 */

DVECTOR_INIT(char, char);

struct adbus_Buffer
{
    /** \privatesection */
    d_Vector(char)  b;
    char            sig[256];
    const char*     sigp;
};

/** Creates a new buffer.
 *  \relates adbus_Buffer 
 */
adbus_Buffer* adbus_buf_new()
{
    adbus_Buffer* b = NEW(adbus_Buffer);
    b->sigp = b->sig;
    return b;
}

/** Frees a buffer.
 *  \relates adbus_Buffer
 */
void adbus_buf_free(adbus_Buffer* b)
{ 
    if (b) {
        dv_free(char, &b->b);
        free(b);
    }
}

/** Returns the current data in the buffer.
 *  \relates adbus_Buffer 
 */
char* adbus_buf_data(const adbus_Buffer* b)
{ return dv_data(&b->b); }

/** Returns the size of the data in the buffer.
 *  \relates adbus_Buffer
 */
size_t adbus_buf_size(const adbus_Buffer* b)
{ return dv_size(&b->b); }

/** Releases the internal buffer and returns it.
 *
 *  \relates adbus_Buffer
 *
 *  After releasing the internal buffer, the user can do as they please with it,
 *  but must free it themselves. Note that since this will release the internal
 *  buffer, you will want to get the buffer size before releasing.
 *
 *  \warning To free the buffer you must use the same version of free as the
 *  library itself. For this reason use of this function is discouraged.
 * 
 */
char* adbus_buf_release(adbus_Buffer* b)
{ 
    b->sig[0] = '\0';
    b->sigp   = b->sig;
    return dv_release(char, &b->b); 
}

/** Clears the internal buffer.
 *
 *  \relates adbus_Buffer
 *
 *  This does not free the internal buffer and thus is much better to use if
 *  you want to reuse the buffer than freeing and newing a new buffer.
 *
 *  A common idiom is to simply clear the buffer right before using it, but not
 *  bothering to clear it after you are finished using it.
 */
void adbus_buf_reset(adbus_Buffer* b)
{ 
    dv_clear(char, &b->b); 
    b->sig[0] = '\0';
    b->sigp   = b->sig;
}

/** Returns the current signature.
 *  \relates adbus_Buffer
 */
const char* adbus_buf_sig(const adbus_Buffer* b, size_t* size)
{ 
    if (size)
        *size = strlen(b->sig);
    return b->sig; 
}

/** Returns the tracked point in the signature.
 *
 *  \relates adbus_Buffer
 *
 *  As data is serialised into the buffer, the serialise functions will track
 *  along the signature. This returns the current point and thus the next
 *  expected field.
 *
 *  This can be very useful for binding to dynamic languages as you can use the
 *  buffer to track the required serialised signature, and then call signext to
 *  figure out which serialise function to call next.
 *
 */
const char* adbus_buf_signext(const adbus_Buffer* b, size_t* size)
{
    if (size)
        *size = strlen(b->sigp);
    return b->sigp;
}

/** Sets the buffer signature.
 *  \relates adbus_Buffer
 */
void adbus_buf_setsig(adbus_Buffer* b, const char* sig, int size)
{
    if (size < 0)
        size = strlen(sig);

    if ((size_t) (size + 1) > sizeof(b->sig)) {
        assert(0);
        return;
    }
    memcpy(b->sig, sig, size + 1);
}

/** Appends to the buffer signature.
 *  \relates adbus_Buffer
 */
void adbus_buf_appendsig(adbus_Buffer* b, const char* sig, int size)
{
    if (size < 0)
        size = strlen(sig);

    size_t existing = strlen(b->sig);
    if (existing + size + 1 > sizeof(b->sig)) {
        assert(0);
        return;
    }
    memcpy(&b->sig[existing], sig, size + 1);
}

/** Reserves a specified min amount of room in the buffer.
 *
 *  \relates adbus_Buffer
 *
 *  This should only be used as a hint when the amount of room needed is known
 *  before adding the data. To actually reserve room at the end of the buffer to then right directly
 *  into use adbus_buf_recvbuf and adbus_buf_recvd.
 *
 *  \sa adbus_buf_recvbuf, adbus_buf_recvd
 */
void adbus_buf_reserve(adbus_Buffer* b, size_t sz)
{ dv_reserve(char, &b->b, sz); }

/** Removes a chunk of data from the buffer
 *  \relates adbus_Buffer
 */
void adbus_buf_remove(adbus_Buffer* b, size_t off, size_t num)
{ dv_erase(char, &b->b, off, num); }

/** Appends a chunk of data to the buffer
 *  \relates adbus_Buffer
 */
void adbus_buf_append(adbus_Buffer* b, const char* data, size_t sz)
{
    char* dest = dv_push(char, &b->b, sz);
    memcpy(dest, data, sz);
}

int adbus_buf_appendvalue(adbus_Buffer* b, adbus_Iterator* i)
{
    const char* data = i->data;
    const char* sig = i->sig;
    if (adbus_iter_value(i))
        return -1;

    adbus_buf_appendsig(b, sig, (int) (i->sig - sig));
    adbus_buf_append(b, data, (int) (i->data - data));
    return 0;
}

/** Returns the next newline terminated line at the start of the
 *  buffer.
 *
 *  \relates adbus_Buffer
 *
 *  \param[in]  b  Buffer to work on
 *  \param[out] sz Length of the returned line.
 *
 *  This is mainly used for SASL auth protocol which is line based.
 *
 *  \warning The returned line will not be null terminated. Instead use the
 *  size out param to determine its size.
 *
 *  \return NULL when there is no complete next line
 *
 */
const char* adbus_buf_line(adbus_Buffer* b, size_t* sz)
{
    char* end = (char*) memchr(dv_data(&b->b), '\n', dv_size(&b->b));
    if (!end)
        return NULL;

    if (sz)
        *sz = end + 1 - dv_data(&b->b);

    return dv_data(&b->b);
}

/** Reserves a buffer to directly append data.
 *
 *  \relates adbus_Buffer
 *
 *  \param[in] b   Buffer to work on
 *  \param[in] len Minimum length of returned buffer.
 *
 *  This is most commonly used for getting a pointer to hand to recv style
 *  functions which write directly into the end of the provided buffer.
 *
 *  \warning You must call adbus_buf_recvd after receiving data to indicate how
 *  much was actually received before calling any other functions on the
 *  buffer.
 *
 *  \sa adbus_buf_recvd
 *
 */
char* adbus_buf_recvbuf(adbus_Buffer* b, size_t len)
{ return dv_push(char, &b->b, len); }


/** Clears out the extra space not used by a adbus_buf_recvbuf().
 *
 *  \relates adbus_Buffer
 *
 *  \param[in] b   Buffer to work on
 *  \param[in] len Length supplied to adbus_buf_recvbuf().
 *  \param[in] recvd Actual amount of data received.
 *
 *  This should be called after receiving data into a buffer gotten via
 *  adbus_buf_recvbuf().
 *
 *  \sa adbus_buf_recvbuf
 */
void adbus_buf_recvd(adbus_Buffer* b, size_t len, adbus_ssize_t recvd)
{
    size_t rx = recvd > 0 ? recvd : 0;
    dv_pop(char, &b->b, len - rx);
}

INLINE void Align(adbus_Buffer* b, int alignment)
{
    int append = ADBUSI_ALIGN(dv_size(&b->b), alignment) - dv_size(&b->b);
    char* dest = dv_push(char, &b->b, append);
    for (int i = 0; i < append; i++) {
        dest[i] = '\0';
    }
}

void adbus_buf_align(adbus_Buffer* b, int alignment)
{ Align(b, alignment); }

INLINE void AlignField(adbus_Buffer* b, char field)
{
    switch (field)
    {
        case 'y': // u8
        case 'g': // signature
        case 'v': // variant
            break;

        case 'n': // i16
        case 'q': // u16
            Align(b, 2);
            break;

        case 'b': // bool
        case 'i': // i32
        case 'u': // u32
        case 's': // string
        case 'o': // object path
        case 'a': // array
            Align(b, 4);
            break;

        case 'x': // i64
        case 't': // u64
        case 'd': // double
        case '(': // struct
        case '{': // dict entry
            Align(b, 8);
            break;

        default:
            assert(0);
            break;
    }
}

INLINE uintptr_t AlignValue(uintptr_t value, char field)
{
    switch (field)
    {
        case 'y': // u8
        case 'g': // signature
        case 'v': // variant
            return value;

        case 'n': // i16
        case 'q': // u16
            return ADBUSI_ALIGN(value, 2);

        case 'b': // bool
        case 'i': // i32
        case 'u': // u32
        case 's': // string
        case 'o': // object path
        case 'a': // array
            return ADBUSI_ALIGN(value, 4);

        case 'x': // i64
        case 't': // u64
        case 'd': // double
        case '(': // struct
        case '{': // dict entry
            return ADBUSI_ALIGN(value, 8);

        default:
            assert(0);
            return value;
    }
}

void adbus_buf_alignfield(adbus_Buffer* b, char field)
{ AlignField(b, field); }

INLINE void Append8(adbus_Buffer* b, uint8_t v)
{
    uint8_t* dest = (uint8_t*) dv_push(char, &b->b, 1);
    *dest = v;
}


INLINE void Append16(adbus_Buffer* b, uint16_t v)
{
    Align(b, 2);
    uint16_t* dest = (uint16_t*) dv_push(char, &b->b, 2);
    *dest = v;
}

INLINE void Append32(adbus_Buffer* b, uint32_t v)
{
    Align(b, 4);
    uint32_t* dest = (uint32_t*) dv_push(char, &b->b, 4);
    *dest = v;
}

INLINE void Append64(adbus_Buffer* b, uint64_t v)
{
    Align(b, 8);
    uint64_t* dest = (uint64_t*) dv_push(char, &b->b, 8);
    *dest = v;
}

#define SIG(b, s) (assert(*b->sigp == s), b->sigp++)

/** Called to end a series of serialised arguments.
 *  \relates adbus_Buffer
 */
void adbus_buf_end(adbus_Buffer* b) {UNUSED(b); assert(*b->sigp == '\0');}

/** Serialises a boolean (dbus sig "b").
 *  \relates adbus_Buffer
 */
void adbus_buf_bool(adbus_Buffer* b, adbus_Bool v){ SIG(b, 'b'); Append32(b, (uint32_t) (v != 0)); }

/** Serialises a uint8_t (dbus sig "y").
 *  \relates adbus_Buffer
 */
void adbus_buf_u8(adbus_Buffer* b, uint8_t v)     { SIG(b, 'y'); Append8(b, v); }

/** Serialises a int16_t (dbus sig "n").
 *  \relates adbus_Buffer
 */
void adbus_buf_i16(adbus_Buffer* b, int16_t v)    { SIG(b, 'n'); Append16(b, *(uint16_t*) &v); }

/** Serialises a uint16_t (dbus sig "q").
 *  \relates adbus_Buffer
 */
void adbus_buf_u16(adbus_Buffer* b, uint16_t v)   { SIG(b, 'q'); Append16(b, v); }

/** Serialises a int32_t (dbus sig "i").
 *  \relates adbus_Buffer
 */
void adbus_buf_i32(adbus_Buffer* b, int32_t v)    { SIG(b, 'i'); Append32(b, *(uint32_t*) &v); }

/** Serialises a uint32_t (dbus sig "u").
 *  \relates adbus_Buffer
 */
void adbus_buf_u32(adbus_Buffer* b, uint32_t v)   { SIG(b, 'u'); Append32(b, v); }

/** Serialises a int64_t (dbus sig "x").
 *  \relates adbus_Buffer
 */
void adbus_buf_i64(adbus_Buffer* b, int64_t v)    { SIG(b, 'x'); Append64(b, *(uint64_t*) &v); }

/** Serialises a uint64_t (dbus sig "t").
 *  \relates adbus_Buffer
 */
void adbus_buf_u64(adbus_Buffer* b, uint64_t v)   { SIG(b, 't'); Append64(b, v); }

/** Serialises a double (dbus sig "d").
 *  \relates adbus_Buffer
 */
void adbus_buf_double(adbus_Buffer* b, double v)  { SIG(b, 'd'); Append64(b, *(uint64_t*) &v); }


/** Serialises a string (dbus sig "s").
 *  \relates adbus_Buffer
 */
void adbus_buf_string(adbus_Buffer* b, const char* str, int size)
{
    SIG(b, 's');
    if (size < 0)
        size = strlen(str);
    assert((size_t) size <= UINT32_MAX);
    Append32(b, (uint32_t) size);
    char* dest = dv_push(char, &b->b, size + 1);
    memcpy(dest, str, size);
    dest[size] = '\0';
}

void adbus_buf_string_vf(adbus_Buffer* b, const char* format, va_list ap)
{
    d_String str = {};
    ds_set_vf(format, ap);
    adbus_buf_string(b, ds_cstr(&str), ds_size(&str));
    ds_free(&str);
}

void adbus_buf_string_f(adbus_Buffer* b, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    adbus_buf_string_vf(b, format, ap);
}

/** Serialises an object path (dbus sig "o").
 *  \relates adbus_Buffer
 */
void adbus_buf_objectpath(adbus_Buffer* b, const char* str, int size)
{
    SIG(b, 'o');
    if (size < 0)
        size = strlen(str);
    assert((size_t) size <= UINT32_MAX);
    Append32(b, (uint32_t) size);
    char* dest = dv_push(char, &b->b, size + 1);
    memcpy(dest, str, size);
    dest[size] = '\0';
}

/** Serialises a signature (dbus sig "g").
 *  \relates adbus_Buffer
 */
void adbus_buf_signature(adbus_Buffer* b, const char* str, int size)
{
    SIG(b, 'g');
    if (size < 0)
        size = strlen(str);
    assert(size < UINT8_MAX);
    Append8(b, (uint8_t) size);
    char* dest = dv_push(char, &b->b, size + 1);
    memcpy(dest, str, size);
    dest[size] = '\0';
}

/** Begins a variant scope (dbus sig "v").
 *  \relates adbus_Buffer
 */
void adbus_buf_beginvariant(adbus_Buffer* b, adbus_BufVariant* v, const char* sig, int size)
{
    SIG(b, 'v');
    if (size < 0)
        size = strlen(sig);
    assert(size < UINT8_MAX);
    Append8(b, (uint8_t) size);
    char* dest = dv_push(char, &b->b, size + 1);
    memcpy(dest, sig, size);
    dest[size] = '\0';
    v->oldsig = b->sigp;
    b->sigp   = sig;
}


/** Ends a variant scope (dbus sig "v").
 *  \relates adbus_Buffer
 */
void adbus_buf_endvariant(adbus_Buffer* b, adbus_BufVariant* v)
{
    b->sigp = v->oldsig;
}

/** Begins an array scope (dbus sig "a").
 *  \relates adbus_Buffer
 */
void adbus_buf_beginarray(adbus_Buffer* b, adbus_BufArray* a)
{
    SIG(b, 'a');
    Append32(b, 0);
    a->szindex = dv_size(&b->b) - 4;
    adbus_buf_alignfield(b, *b->sigp);
    a->dataindex = dv_size(&b->b);
    a->sigbegin = b->sigp;
    a->sigend = adbus_nextarg(b->sigp);
}

/** Begins an array entry (dbus sig "a").
 *  \relates adbus_Buffer
 */
void adbus_buf_arrayentry(adbus_Buffer* b, adbus_BufArray* a)
{
    b->sigp = a->sigbegin;
}

void adbus_buf_checkarrayentry(adbus_Buffer* b, adbus_BufArray* a)
{
    if (b->sigp == a->sigend)
        b->sigp = a->sigbegin;
}

/** Ends an array scope (dbus sig "a").
 *  \relates adbus_Buffer
 */
void adbus_buf_endarray(adbus_Buffer* b, adbus_BufArray* a)
{
    uint32_t* sz = (uint32_t*) &dv_a(&b->b, a->szindex);
    *sz = dv_size(&b->b) - a->dataindex;
    b->sigp = a->sigend;
}

/** Begins a dict entry (dbus sig "{").
 *  \relates adbus_Buffer
 */
void adbus_buf_begindictentry(adbus_Buffer* b)  { SIG(b, '{'); Align(b, 8); }

/** Ends a dict entry (dbus sig "}").
 *  \relates adbus_Buffer
 */
void adbus_buf_enddictentry(adbus_Buffer* b)    { SIG(b, '{'); }

/** Begins a struct (dbus sig "(").
 *  \relates adbus_Buffer
 */
void adbus_buf_beginstruct(adbus_Buffer* b)     { SIG(b, '('); Align(b, 8); }

/** Ends a struct (dbus sig ")").
 *  \relates adbus_Buffer
 */
void adbus_buf_endstruct(adbus_Buffer* b)       { SIG(b, ')'); }





