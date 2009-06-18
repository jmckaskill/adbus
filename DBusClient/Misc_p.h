#pragma once

#include "Common.h"

#ifndef __cplusplus
#define true 1
#define false 0
#define bool int
#endif

//-----------------------------------------------------------------------------

extern const char _DBusNativeEndianness;

int _DBusRequiredAlignment(char type);

//-----------------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct _DBusMessageHeader
{
  // 8 byte begin padding
  uint8_t   endianness;
  uint8_t   type;
  uint8_t   flags;
  uint8_t   version;
  uint32_t  length;
  uint32_t  serial;
};

struct _DBusMessageExtendedHeader
{
  struct _DBusMessageHeader header;

  // HeaderFields are a(yv)
  uint32_t  headerFieldLength;
  // Alignment of header data is 8 byte since array element is a struct
  // sizeof(MessageHeader) == 16 therefore no beginning padding necessary
  
  // uint8_t headerData[headerFieldLength];
  // uint8_t headerEndPadding -- pads to 8 byte
};
#pragma pack(pop)

//-----------------------------------------------------------------------------


#define ASSERT_RETURN(x) ASSERT(x); if (!(x)) return;

/*
 * Realloc the buffer pointed at by variable 'x' so that it can hold
 * at least 'nr' entries; the number of entries currently allocated
 * is 'alloc', using the standard growing factor alloc_nr() macro.
 *
 * DO NOT USE any expression with side-effect for 'x' or 'alloc'.
 */
#define alloc_nr(x) (((x)+16)*3/2)
#define ALLOC_GROW(x, nr, alloc) \
	do { \
		if ((nr) > alloc) { \
			if (alloc_nr(alloc) < (nr)) \
				alloc = (nr); \
			else \
				alloc = alloc_nr(alloc); \
			x = realloc((x), alloc * sizeof(*(x))); \
		} \
	} while(0)


//-----------------------------------------------------------------------------

bool _DBusIsValidObjectPath(const char* str, size_t len);
bool _DBusIsValidInterfaceName(const char* str, size_t len);
bool _DBusIsValidBusName(const char* str, size_t len);
bool _DBusIsValidMemberName(const char* str, size_t len);
bool _DBusHasNullByte(const char* str, size_t len);
bool _DBusIsValidUtf8(const char* str, size_t len);

//-----------------------------------------------------------------------------


