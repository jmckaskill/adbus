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

#ifndef ADBUS_ITERATOR_H
#define ADBUS_ITERATOR_H

#include <adbus.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/** \struct adbus_Iterator
 *  
 *  \brief Iterator over serialised dbus data
 *
 *  \warning adbus_Iterator will only work on data in the native endianness.
 *  For non native data, it should first be endian flipped using
 *  adbus_flip_data() or adbus_flip_value().
 *
 *  \warning adbus_Iterator requires that the data is 8 byte aligned. This is
 *  easiest to achieve by copying the data into an adbus_Buffer.
 *
 *  All iterate functions return the data via a pointer out param and return
 *  an int. The returned int is zero on success and non-zero on a parse
 *  failure or invalid data.
 *
 *  The iterate functions will track along the signature in the iterator, but
 *  expect that the signature has already been checked (they will assert when
 *  there is a signature mismatch). For argument iteration with built in
 *  argument checking see the check functions (eg adbus_check_bool()).
 *
 *  The functions are mainly designed for speed, will do minimal data
 *  checking, and are all inlined. The only real checking done is to check
 *  null terminators for strings.
 *
 *  \section array Arrays
 *
 *  Arrays can be iterated over in two fashions. You can either use
 *  adbus_buf_inarray() in a loop to pull out each entry in the array or
 *  simply copy the data directly out of the data and size members of
 *  adbus_IterArray.
 *
 *  \warning When pulling out array data directly or referencing it directly
 *  you need to ensure that the target type has the exact same alignment as
 *  the marshalled data.
 *
 *  For example using adbus_buf_inarray() to iterator over "au" (an array of
 *  uint32s);
 *
 *  \code
 *  std::vector<uint32_t> vec;
 *  adbus_IterArray a;
 *  if (adbus_iter_beginarray(buf, &a))
 *      return -1;
 *  while (adbus_iter_inarray(buf, &a))
 *  {
 *      uint32_t v;
 *      if (adbus_iter_u32(buf, &v))
 *          return -1;
 *      vec.push_back(v);
 *  }
 *  if (adbus_iter_endarray(buf, &a))
 *      return -1;
 *  \endcode
 *
 *  Equivalently by pulling the array data out directly:
 *
 *  \code
 *  std::vector<uint32_t> vec;
 *  adbus_IterArray a;
 *  if (adbus_iter_beginarray(iter, &a))
 *      return -1;
 *  vec.insert(vec.end(), (uint32_t*) a.data, (uint32_t*) (a.data + a.size));
 *  if (adbus_iter_endarray(iter, &a))
 *      return -1;
 *  \endcode
 *
 *  adbus_IterArray is an exposed type with the following members:
 *
 *  \code
 *  struct adbus_IterArray
 *  {
 *      const char* sig;    // signature of array entry - not null terminated
 *      const char* data;   // beginning of array data
 *      size_t size;        // size of array data
 *  };
 *  \endcode
 *
 *  \section dictentry Dict Entries
 *
 *  Dict entries can only be used as a scope directly inside an array. Thus
 *  the signature will always look like a{...}. The begin/end dictentry calls
 *  should be inside an inarray loop.
 *
 *  For example to iterate over "a{is}" (a map of int to string):
 *
 *  \code
 *  std::map<int, std::string> map;
 *  adbus_IterArray a;
 *  if (adbus_iter_beginarray(iter, &a))
 *      return -1;
 *  while (adbus_iter_inarray(iter, &a))
 *  {
 *      int32_t i32;
 *      const char* str;
 *      if (adbus_iter_begindictentry(iter))
 *          return -1;
 *      if (adbus_iter_i32(iter, &i32))
 *          return -1;
 *      if (adbus_iter_string(iter, &str, NULL))
 *          return -1;
 *      if (adbus_iter_enddictentry(iter))
 *          return -1;
 *      map[i32] = str;
 *  }
 *  if (adbus_iter_endarray(iter, &a))
 *      return -1;
 *  \endcode
 *
 *
 *
 *  \section struct Structs
 *
 *  Structs can be iterated over by begin and end functions with no scoped
 *  data.
 *
 *  For example to iterate over "(iis)" (a struct of int, int, string):
 *
 *  \code
 *  int32_t i1, i2;
 *  const char* str;
 *  if (adbus_iter_beginstruct(iter))
 *      return -1;
 *  if (adbus_iter_i32(iter, &i1))
 *      return -1;
 *  if (adbus_iter_i32(iter, &i1))
 *      return -1;
 *  if (adbus_iter_string(iter, &str))
 *      return -1;
 *  if (adbus_iter_endstruct(iter))
 *      return -1;
 *  \endcode
 *
 *  
 *  \section variant Variants
 *
 *  Variants have begin and functions and scoped data. After the begin variant
 *  call, the sig member of the iterator is set to the signature of the
 *  variant data. This can be used to check the signature of the variant,
 *  which must be checked before iterating over specific value types.
 *
 *  adbus_IterVariant is an exposed type with the following members:
 *
 *  \code
 *  struct adbus_IterVariant
 *  {
 *      const char* origsig;
 *      const char* sig;
 *      const char* data;
 *      size_t      size;
 *  };
 *  \endcode
 *
 *  For example to iterate over a variant which we expect to be either "s"
 *  (string) or "u" (uint32_t):
 *
 *  \code
 *  uint32_t u;
 *  const char* str;
 *  adbus_IterVariant v;
 *  if (adbus_iter_beginvariant(iter, &v))
 *      return -1;
 *  if (strcmp(iter->sig, "u") == 0)
 *  {
 *      // We have a u32 variant
 *      if (adbus_iter_u32(iter->sig, &u))
 *          return -1;
 *  }
 *  else if (strcmp(iter->sig, "s") == 0)
 *  {
 *      // We have a string variant
 *      if (adbus_iter_string(iter->sig, &str))
 *          return -1;
 *  }
 *  else
 *  {
 *      // We have some other variant
 *      return -1;
 *  }
 *  if (adbus_iter_endvariant(iter, &v))
 *      return -1;
 *  \endcode
 *
 *  Alternatively if we don't care too much about the type just yet, we can
 *  copy around the variant in its marshalled form with the associated
 *  signature. We can do this by using adbus_iter_value() to skip to the end
 *  of the variant and finishing it. Then the data, size, and sig members of
 *  the adbus_IterVariant will have the needed info.
 *
 *  For example:
 *
 *  \code
 *  adbus_IterVariant v;
 *  if (adbus_iter_beginvariant(iter, &v))
 *      return -1;
 *  if (adbus_iter_value(iter))
 *      return -1;
 *  if (adbus_iter_endvariant(iter, &v))
 *      return -1;
 *
 *  adbus_Buffer* buf = adbus_buf_new();
 *  adbus_buf_setsig(buf, v.sig, -1);
 *  adbus_buf_append(buf, v.data, v.size);
 *
 *  // some time later
 *  adbus_Iterator iter;
 *  adbus_iter_buffer(&iter, buf);
 *  // iterate over the data ...
 *
 *  // once we are done with the data
 *  adbus_buf_free(buf);
 *  \endcode
 *
 *  To pull out the information later we simply setup a new iterator and
 *  iterate over the copy of the data. Note that the data must still by 8 byte
 *  aligned and thus it is often easiest to cart around the data in an
 *  adbus_Buffer.
 *
 *  This method has the advantage of keeping the data in dbus marshalled form
 *  and thus can hold any dbus type.
 *
 */

struct adbus_Iterator
{
    const char*     data;   /**< Current data pointer */
    size_t          size;   /**< Data left */
    const char*     sig;    /**< Current signature pointer */
};

/* This alignment thing is from ORBit2
 * Align a value upward to a boundary, expressed as a number of bytes.
 * E.g. align to an 8-byte boundary with argument of 8.
 */

/*
 *   (this + boundary - 1)
 *          &
 *    ~(boundary - 1)
 */

#define ADBUS_ALIGN(PTR, BOUNDARY) \
  ((((uintptr_t) (PTR)) + (((uintptr_t) (BOUNDARY)) - 1)) & (~(((uintptr_t)(BOUNDARY))-1)))


ADBUS_INLINE int adbus_iter_align(adbus_Iterator* i, int alignment)
{
    size_t diff = (char*) ADBUS_ALIGN(i->data, alignment) - i->data;
    if (i->size < diff)
        return -1;

    i->data += diff;
    i->size -= diff;
    return 0;
}

ADBUS_INLINE int adbus_iter_alignfield(adbus_Iterator* i, char field)
{
    switch (field)
    {
        case 'y': /* u8 */
        case 'g': /* signature */
        case 'v': /* variant */
            return 0;

        case 'n': /* i16 */
        case 'q': /* u16 */
            return adbus_iter_align(i,2);

        case 'b': /* bool */
        case 'i': /* i32 */
        case 'u': /* u32 */
        case 's': /* string */
        case 'o': /* object path */
        case 'a': /* array */
            return adbus_iter_align(i,4);

        case 'x': /* i64 */
        case 't': /* u64 */
        case 'd': /* double */
        case '(': /* struct */
        case '{': /* dict entry */
            return adbus_iter_align(i,8);

        default:
            assert(0);
            return -1;
    }
}

ADBUS_INLINE int adbusI_iter_sig(adbus_Iterator* i, char field)
{
    if (*i->sig++ != field) {
        assert(0);
        return -1;
    }
    return 0;
}

ADBUS_INLINE int adbusI_iter_get8(adbus_Iterator* i, uint8_t* v)
{
    if (i->size < 1)
        return -1;

    if (v)
        *v = *(const uint8_t*) i->data;
    i->data += 1;
    i->size -= 1;
    return 0;
}

ADBUS_INLINE int adbusI_iter_get16(adbus_Iterator* i, uint16_t* v)
{
    if (adbus_iter_align(i, 2) || i->size < 2)
        return -1;

    if (v)
        *v = *(const uint16_t*) i->data;
    i->data += 2;
    i->size -= 2;
    return 0;
}

ADBUS_INLINE int adbusI_iter_get32(adbus_Iterator* i, uint32_t* v)
{
    if (adbus_iter_align(i, 4) || i->size < 4)
        return -1;

    if (v)
        *v = *(const uint32_t*) i->data;
    i->data += 4;
    i->size -= 4;
    return 0;
}

ADBUS_INLINE int adbusI_iter_get64(adbus_Iterator* i, uint64_t* v)
{
    if (adbus_iter_align(i, 8) || i->size < 8)
        return -1;

    if (v)
        *v = *(const uint64_t*) i->data;
    i->data += 8;
    i->size -= 8;
    return 0;
}

/** Pulls out a boolean (dbus sig "b").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_bool(adbus_Iterator* i, adbus_Bool* v)
{ 
    uint32_t u32;
    if (adbusI_iter_sig(i, 'b') || adbusI_iter_get32(i, &u32))
        return -1;
    *v = (u32 != 0);
    return 0;
}

/** Pulls out a uint8_t (dbus sig "y").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_u8(adbus_Iterator* i, uint8_t* v)
{ return adbusI_iter_sig(i, 'y') || adbusI_iter_get8(i, v); }

/** Pulls out a int16_t (dbus sig "n").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_i16(adbus_Iterator* i, int16_t* v)
{ return adbusI_iter_sig(i, 'n') || adbusI_iter_get16(i, (uint16_t*) v); }

/** Pulls out a uint16_t (dbus sig "q").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_u16(adbus_Iterator* i, uint16_t* v)
{ return adbusI_iter_sig(i, 'q') || adbusI_iter_get16(i, v); }

/** Pulls out a int32_t (dbus sig "i").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_i32(adbus_Iterator* i, int32_t* v)
{ return adbusI_iter_sig(i, 'i') || adbusI_iter_get32(i, (uint32_t*) v); }

/** Pulls out a uint32_t (dbus sig "u").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_u32(adbus_Iterator* i, uint32_t* v)
{ return adbusI_iter_sig(i, 'u') || adbusI_iter_get32(i, v); }

/** Pulls out a int64_t (dbus sig "x").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_i64(adbus_Iterator* i, int64_t* v)
{ return adbusI_iter_sig(i, 'x') || adbusI_iter_get64(i, (uint64_t*) v); }

/** Pulls out a uint64_t (dbus sig "t").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_u64(adbus_Iterator* i, uint64_t* v)
{ return adbusI_iter_sig(i, 't') || adbusI_iter_get64(i, v); }

/** Pulls out a double (dbus sig "d").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_double(adbus_Iterator* i, double* v)
{ return adbusI_iter_sig(i, 'd') || adbusI_iter_get64(i, (uint64_t*) v); }

ADBUS_INLINE int adbusI_iter_getstring(adbus_Iterator* i, size_t strsz, const char** pstr, size_t* pstrsz)
{
    const char* str = i->data;
    if (i->size < (size_t) (strsz + 1))
        return -1;
    if (memchr(str, '\0', strsz + 1) != str + strsz)
        return -1;
    i->data += strsz + 1;
    i->size -= strsz + 1;
    if (pstr)
        *pstr = str;
    if (pstrsz)
        *pstrsz = strsz;
    return 0;
}

/** Pulls out a string (dbus sig "s").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_string(adbus_Iterator* i, const char** str, size_t* strsz)
{
    uint32_t len;
    return adbusI_iter_sig(i, 's')
        || adbusI_iter_get32(i, &len)
        || adbusI_iter_getstring(i, len, str, strsz);
}

/** Pulls out a object path (dbus sig "o").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_objectpath(adbus_Iterator* i, const char** str, size_t* strsz)
{
    uint32_t len;
    return adbusI_iter_sig(i, 'o')
        || adbusI_iter_get32(i, &len)
        || adbusI_iter_getstring(i, len, str, strsz);
}

/** Pulls out a signature (dbus sig "g").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_signature(adbus_Iterator* i, const char** str, size_t* strsz)
{
    uint8_t len;
    return adbusI_iter_sig(i, 'g')
        || adbusI_iter_get8(i, &len)
        || adbusI_iter_getstring(i, len, str, strsz);
}

/** Data structure used by the array iterate functions.
 *
 *  \sa adbus_Iterator, adbus_iter_beginarray(), adbus_iter_inarray(),
 *  adbus_iter_endarray()
 */
struct adbus_IterArray
{
    const char* sig;    /**< Signature of the array data - not null terminated */
    size_t sigsz;       /**< Size of the array data signature */
    const char* data;   /**< Beginning of the array data */
    size_t size;        /**< Size of the array data */
};

ADBUS_API const char* adbus_nextarg(const char* sig);

#define ADBUS_MAXIMUM_ARRAY_LENGTH 67108864
/** Begins an array scope (dbus sig "a").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_beginarray(adbus_Iterator* i, adbus_IterArray* a)
{
    uint32_t len;
    const char* sigend;
    if (adbusI_iter_sig(i, 'a') || adbusI_iter_get32(i, &len))
        return -1;

    if (len > ADBUS_MAXIMUM_ARRAY_LENGTH)
        return -1;

    if (adbus_iter_alignfield(i, *i->sig))
        return -1;

    sigend = adbus_nextarg(i->sig);
    if (sigend == NULL)
        return -1;

    a->data  = i->data;
    a->size  = len;
    a->sig   = i->sig;
    a->sigsz = sigend - a->sig;
    return 0;
}

/** Checks that the iterator is still inside the array.
 *  \relates adbus_Iterator
 */
ADBUS_INLINE adbus_Bool adbus_iter_inarray(adbus_Iterator* i, adbus_IterArray* a)
{
    if (i->data < a->data + a->size) {
        i->sig = a->sig;
        return 1;
    } else {
        return 0;
    }
}

/** Ends an array scope (dbus sig "a").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_endarray(adbus_Iterator* i, adbus_IterArray* a)
{
    i->data = a->data + a->size;
    i->sig = a->sig + a->sigsz;
    return 0;
}

/** Begins a dict entry scope (dbus sig "{").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_begindictentry(adbus_Iterator* i)
{ return adbusI_iter_sig(i, '{') || adbus_iter_align(i, 8); }

/** Ends a dict entry scope (dbus sig "}").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_enddictentry(adbus_Iterator* i)
{ return adbusI_iter_sig(i, '}'); }

/** Begins a struct scope (dbus sig "(").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_beginstruct(adbus_Iterator* i)
{ return adbusI_iter_sig(i, '(') || adbus_iter_align(i, 8); }

/** Ends a struct scope (dbus sig ")").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_endstruct(adbus_Iterator* i)
{ return adbusI_iter_sig(i, ')'); }

/** Data structure used by the variant iterator functions.
 *  \sa adbus_Iterator, adbus_iter_beginvariant(), adbus_iter_endvariant()
 */
struct adbus_IterVariant
{
    const char* origsig;    /**< Signature of the next argument */
    const char* sig;        /**< Signature of the variant data */
    const char* data;       /**< Beginning of the variant data */
    size_t size;            /**< Size of the variant data */
};

/** Begins a variant scope (dbus sig "v").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_beginvariant(adbus_Iterator* i, adbus_IterVariant* v)
{ 
    uint8_t len;
    const char* vsig;
    if (    adbusI_iter_sig(i, 'v')
        ||  adbusI_iter_get8(i, &len)
        ||  adbusI_iter_getstring(i, len, &vsig, NULL)
        ||  adbus_iter_alignfield(i, *vsig))
    {
        return -1;
    }

    v->origsig = i->sig;
    v->data = i->data;
    v->sig = vsig;
    i->sig = vsig;
    return 0;
}

/** Ends a variant scope (dbus sig "v").
 *  \relates adbus_Iterator
 */
ADBUS_INLINE int adbus_iter_endvariant(adbus_Iterator* i, adbus_IterVariant* v)
{
    if (*i->sig)
        return -1;
    i->sig = v->origsig;
    v->size = i->data - v->data;
    return 0;
}


#endif /* ADBUS_ITERATOR_H */

