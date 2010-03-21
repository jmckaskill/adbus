
adbus
=====

The main documentation is in doxygen and can be built by running doxygen in
the root directory or viewed online at <http://jmckaskill.github.com/adbus/>.

About DBus the Protocol
-----------------------

TODO: Give brief about dbus - for now see
<http://www.freedesktop.org/wiki/Software/dbus> and
<http://doc.trolltech.com/4.6/intro-to-dbus.html>.

About adbus
-----------

adbus is designed to be a portable C implementation of both a dbus client
and bus server, and bound into higher level languages or used directly
from C/C++ with ease.

It's written in the common subset of ISO C and C++ and uses only the CRT
except for the optional adbus_Socket module which uses the BSD socket API
and some atomic operations found in adbus/misc.h.

The API as a whole is thread-safe (ie has no global data) but most objects
can only be used on a single thread. The API especially adbus_Connection
is heavily callback based and thus there are some restrictions on what
functions can be called in callbacks to avoid reentrancy problems.
Notably:

- Any number of adbus_Connection services can be removed at any time.
- Services can be added in a message callback but not in release
callbacks.
- Neither the parse nor dispatch functions of either the connection or
server can be called in any callbacks. This is scheduled to be addressed
soon.

The multithreading support is incomplete and untested. For the moment
being it would be unwise to use a connection on multiple threads, but this
will be addressed soon.





Building
--------

The linux build is built using tup available from <http://gittup.org/tup/>.

To build tup:

    git clone git://gittup.org/tup.git
    cd tup
    ./bootstrap.sh

To build adbus with tup:

    git clone git://github.com/jmckaskill/adbus.git
    cd adbus
    ../tup/tup init
    ../tup/tup upd

Building on other platforms or manually with gcc is fairly
straightforward:

    gcc -o adbus.so -Iinclude/c -fPIC --std=gnu99 adbus/ *.c dmem/ *.c

For MSVC you will need the provided stdint.h in deps/msvc/stdint.h.




Design
------

The adbus API is composed of a number of modules centred around the major
types. These modules can be grouped into 4 groups:

1. Low level modules - This mainly deals with building/parsing messages
and serialising/deserialising dbus data. This incudes:
    - adbus_Iterator - Iterator to deserialise dbus data.
    - adbus_Buffer - Generic buffer and means to serialise dbus data.
    - adbus_Message - Message parsing functions.
    - adbus_MsgFactory - Building messages
2. Client API
    - adbus_Connection - Message dispatcher and handles match, reply and
    bind registrations
        - adbus_Match - Used for generic matches (mostly for
        matching signals) and can be pushed through to the bus
        - adbus_Reply - Registering a callback for a reply to a
        method call (both return and error messages)
        - adbus_Bind - Registering an adbus_Interface on a given
        path 
    - adbus_Interface - Wrapper around a dbus interface used when binding
    an interface to a path.
    - adbus_CbData - Functions and data structure used for message
    callbacks.
3. Client utilities
    - adbus_State - Connection service management - \b strongly advised
    when using the API directly.
    - adbus_Proxy - Proxy module to ease interacting with a specific
    remote object.
    - adbus_Auth - Simple SASL implementation for both client and server
    side.
    - adbus_Signal - Helper for emitting signals from C.
    - adbus_Socket - Helper functions to create sockets using the BSD
    sockets API.
    - adbus_Misc - Miscellaneous functions
4. Server API
    - adbus_Server - Simple single threaded dbus bus server.



Full Example
------------

Example blocking BSD sockets main loop.

    static void SetupConnection(adbus_Connection* c)
    {
        // Do connection setup
    }

    static adbus_ssize_t SendMessage(void* user, adbus_Message* m)
    { return send(*(adbus_Socket*) user, m->data, m->size, 0); }

    # define RECV_SIZE 64 * 1024
    int main(void)
    {
        adbus_Buffer* buffer;
        adbus_Connection* connection;
        adbus_Socket socket;
        adbus_ConnectionCallbacks callbacks;

        // Connect and authenticate
        buffer = adbus_buf_new();
        socket = adbus_sock_connect(ADBUS_DEFAULT_BUS);
        if (socket == ADBUS_SOCK_INVALID || adbus_sock_cauth(socket, buffer))
            return 1;

        // Create and setup connection
        memset(&callbacks, 0, sizeof(callbacks));
        callbacks.send_message = &SendMessage;
        connection = adbus_conn_new(&callbacks, &socket);
        SetupConnection(connection);

        // Connect to bus server
        adbus_conn_connect(connection, NULL, NULL);

        while (1) {
            // Get the next chunk of data
            char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
            adbus_ssize_t recvd = recv(socket, buffer, RECV_SIZE, 0);
            adbus_buf_recvd(buffer, RECV_SIZE, recvd);

            if (recvd < 0)
                return 2;

            // Dispatch received data
            if (adbus_conn_parse(connection, buffer))
                return 3;
        }
    }



