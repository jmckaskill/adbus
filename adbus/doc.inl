#error This file is only meant to be included by doxygen.

/** \mainpage adbus
 *
 *
 *  \section dbus About DBus the Protocol
 *
 *  \todo Give brief about dbus - for now see
 *  http://www.freedesktop.org/wiki/Software/dbus and
 *  http://doc.trolltech.com/4.6/intro-to-dbus.html
 *
 *
 *
 *  \section about About adbus
 *
 *  adbus is designed to be a portable C implementation of both a dbus client
 *  and bus server, and bound into higher level languages or used directly
 *  from C/C++ with ease.
 *
 *  It's written in the common subset of ISO C and C++ and uses only the CRT
 *  except for the optional adbus_Socket module which uses the BSD socket API
 *  and some atomic operations found in adbus/misc.h.
 *
 *  The API as a whole is thread-safe (ie has no global data) but most objects
 *  can only be used on a single thread. The API especially adbus_Connection
 *  is heavily callback based and thus there are some restrictions on what
 *  functions can be called in callbacks to avoid reentrancy problems.
 *  Notably:
 *
 *  - Any number of adbus_Connection services can be removed at any time.
 *  - Services can be added in a message callback but not in release
 *  callbacks.
 *  - Neither the parse nor dispatch functions of either the connection or
 *  server can be called in any callbacks. This is scheduled to be addressed
 *  soon.
 *
 *  The multithreading support is incomplete and untested. For the moment
 *  being it would be unwise to use a connection on multiple threads, but this
 *  will be addressed soon.
 *
 *
 *  
 *  \section build Building
 *
 *  The linux build is built using tup available from http://gittup.org/tup/ .
 *
 *  To build tup:
 *  \code
 *  git clone git://gittup.org/tup.git
 *  cd tup
 *  ./bootstrap.sh
 *  \endcode
 *
 *  To build adbus with tup:
 *  \code
 *  git clone git://github.com/jmckaskill/adbus.git
 *  cd adbus
 *  ../tup/tup init
 *  ../tup/tup upd
 *  \endcode
 *
 *  Building on other platforms or manually with gcc is fairly
 *  straightforward:
 *
 *  \code
 *  gcc -o adbus.so -Iinclude/c -fPIC --std=gnu99 adbus/ *.c dmem/ *.c
 *  \endcode
 *
 *  For MSVC you will need the provided stdint.h in deps/msvc/stdint.h.
 *
 *
 *
 *
 *  \section design Design
 *
 *  The adbus API is composed of a number of modules centred around the major
 *  types. These modules can be grouped into 4 groups:
 *
 *  -# Low level modules - This mainly deals with building/parsing messages
 *  and serialising/deserialising dbus data. This incudes:
 *      - adbus_Iterator - Iterator to deserialise dbus data.
 *      - adbus_Buffer - Generic buffer and means to serialise dbus data.
 *      - adbus_Message - Message parsing functions.
 *      - adbus_MsgFactory - Building messages
 *  -# Client API
 *      - adbus_Connection - Message dispatcher and handles match, reply and
 *      bind registrations
 *          - adbus_Match - Used for generic matches (mostly for
 *          matching signals) and can be pushed through to the bus
 *          - adbus_Reply - Registering a callback for a reply to a
 *          method call (both return and error messages)
 *          - adbus_Bind - Registering an adbus_Interface on a given
 *          path 
 *      - adbus_Interface - Wrapper around a dbus interface used when binding
 *      an interface to a path.
 *      - adbus_CbData - Functions and data structure used for message
 *      callbacks.
 *  -# Client utilities
 *      - adbus_State - Connection service management - \b strongly advised
 *      when using the API directly.
 *      - adbus_Proxy - Proxy module to ease interacting with a specific
 *      remote object.
 *      - adbus_Auth - Simple SASL implementation for both client and server
 *      side.
 *      - adbus_Signal - Helper for emitting signals from C.
 *      - \ref adbus_Socket - Helper functions to create sockets using the BSD
 *      sockets API.
 *      - \ref adbus_Misc - Miscellaneous functions
 *  -# Server API
 *      - adbus_Server - Simple single threaded dbus bus server.
 *
 *
 *
 *  \section Full Example
 *
 *  Example blocking BSD sockets main loop.
 *
 *  \code
 *  static void SetupConnection(adbus_Connection* c)
 *  {
 *      // Do connection setup
 *  }
 *
 *  static adbus_ssize_t SendMessage(void* user, adbus_Message* m)
 *  { return send(*(adbus_Socket*) user, m->data, m->size, 0); }
 *
 *  # define RECV_SIZE 64 * 1024
 *  int main(void)
 *  {
 *      adbus_Buffer* buffer;
 *      adbus_Connection* connection;
 *      adbus_Socket socket;
 *      adbus_ConnectionCallbacks callbacks;
 *
 *      // Connect and authenticate
 *      buffer = adbus_buf_new();
 *      socket = adbus_sock_connect(ADBUS_DEFAULT_BUS);
 *      if (socket == ADBUS_SOCK_INVALID || adbus_sock_cauth(socket, buffer))
 *          return 1;
 *
 *      // Create and setup connection
 *      memset(&callbacks, 0, sizeof(callbacks));
 *      callbacks.send_message = &SendMessage;
 *      connection = adbus_conn_new(&callbacks, &socket);
 *      SetupConnection(connection);
 *
 *      // Connect to bus server
 *      adbus_conn_connect(connection, NULL, NULL);
 *
 *      while (1) {
 *          // Get the next chunk of data
 *          char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
 *          adbus_ssize_t recvd = recv(socket, buffer, RECV_SIZE, 0);
 *          adbus_buf_recvd(buffer, RECV_SIZE, recvd);
 *
 *          if (recvd < 0)
 *              return 2;
 *
 *          // Dispatch received data
 *          if (adbus_conn_parse(connection, buffer))
 *              return 3;
 *      }
 *  }
 *  \endcode
 *
 */


/* Definition table of the various dbus types.
 *
 * Pulled from http://dbus.freedesktop.org/doc/dbus-specification.html
 *
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | Name        | Code             | Description                          | Alignment   | Encoding                                      |
 * +=============+==================+======================================+=============+===============================================+
 * | INVALID     | 0 (ASCII NUL)    | Not a valid type code, used to       | N/A         | Not applicable; cannot be marshaled.          |
 * |             |                  | terminate signatures                 |             |                                               |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | BYTE        | 121 (ASCII 'y')  | 8-bit unsigned integer               | 1           | A single 8-bit byte.                          |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | BOOLEAN     | 98 (ASCII 'b')   | Boolean value, 0 is FALSE and 1      | 4           | As for UINT32, but only 0 and 1 are valid     |
 * |             |                  | is TRUE. Everything else is invalid. |             | values.                                       |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT16       | 110 (ASCII 'n')  | 16-bit signed integer                | 2           | 16-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT16      | 113 (ASCII 'q')  | 16-bit unsigned integer              | 2           | 16-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT32       | 105 (ASCII 'i')  | 32-bit signed integer                | 4           | 32-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT32      | 117 (ASCII 'u')  | 32-bit unsigned integer              | 4           | 32-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | INT64       | 120 (ASCII 'x')  | 64-bit signed integer                | 8           | 64-bit signed integer in the message's byte   |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | UINT64      | 116 (ASCII 't')  | 64-bit unsigned integer              | 8           | 64-bit unsigned integer in the message's byte |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | DOUBLE      | 100 (ASCII 'd')  | IEEE 754 double                      | 8           | 64-bit IEEE 754 double in the message's byte  |
 * |             |                  |                                      |             | order.                                        |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | STRING      | 115 (ASCII 's')  | UTF-8 string (must be valid UTF-8).  | 4 (for      | A UINT32 indicating the string's length in    |
 * |             |                  | Must be nul terminated and contain   | the length) | bytes excluding its terminating nul, followed |
 * |             |                  | no other nul bytes.                  |             | by non-nul string data of the given length,   |
 * |             |                  |                                      |             | followed by a terminating nul byte.           |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | OBJECT_PATH | 111 (ASCII 'o')  | Name of an object instance           | 4 (for      | Exactly the same as STRING except the content |
 * |             |                  |                                      | the length) | must be a valid object path (see below).      |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | SIGNATURE   | 103 (ASCII 'g')  | A type signature                     | 1           | The same as STRING except the length is a     |
 * |             |                  |                                      |             | single byte (thus signatures have a maximum   |
 * |             |                  |                                      |             | length of 255) and the content must be a      |
 * |             |                  |                                      |             | valid signature (see below).                  |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | ARRAY       | 97 (ASCII 'a')   | Array                                | 4 (for      | A UINT32 giving the length of the array data  |
 * |             |                  |                                      | the length) | in bytes, followed by alignment padding to    |
 * |             |                  |                                      |             | the alignment boundary of the array element   |
 * |             |                  |                                      |             | type, followed by each array element. The     |
 * |             |                  |                                      |             | array length is from the end of the alignment |
 * |             |                  |                                      |             | padding to the end of the last element, i.e.  |
 * |             |                  |                                      |             | it does not include the padding after the     |
 * |             |                  |                                      |             | length, or any padding after the last         |
 * |             |                  |                                      |             | element. Arrays have a maximum length defined |
 * |             |                  |                                      |             | to be 2 to the 26th power or 67108864.        |
 * |             |                  |                                      |             | Implementations must not send or accept       |
 * |             |                  |                                      |             | arrays exceeding this length.                 |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | STRUCT      | 114 (ASCII 'r'), | Struct                               | 8           | A struct must start on an 8-byte boundary     |
 * |             | 40 (ASCII '('),  |                                      |             | regardless of the type of the struct fields.  |
 * |             | 41 (ASCII ')')   |                                      |             | The struct value consists of each field       |
 * |             |                  |                                      |             | marshaled in sequence starting from that      |
 * |             |                  |                                      |             | 8-byte alignment boundary.                    |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | VARIANT     | 118 (ASCII 'v')  | Variant type (the type of the        | 1           | A variant type has a marshaled SIGNATURE      |
 * |             |                  | value is part of the value           | (alignment  | followed by a marshaled value with the type   |
 * |             |                  | itself)                              | of          | given in the signature. Unlike a message      |
 * |             |                  |                                      | signature)  | signature, the variant signature can contain  |
 * |             |                  |                                      |             | only a single complete type.  So "i" is OK,   |
 * |             |                  |                                      |             | "ii" is not.                                  |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 * | DICT_ENTRY  | 101 (ASCII 'e'), | Entry in a dict or map (array        | 8           | Identical to STRUCT.                          |
 * |             | 123 (ASCII '{'), | of key-value pairs)                  |             |                                               |
 * |             | 125 (ASCII '}')  |                                      |             |                                               |
 * +-------------+------------------+--------------------------------------+-------------+-----------------------------------------------+
 */


