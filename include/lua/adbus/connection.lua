-------------------------------------------------------------------------------
-- vim: ts=4 sw=4 sts=4 et tw=78
--
-- Copyright (c) 2009 James R. McKaskill
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the "Software"),
-- to deal in the Software without restriction, including without limitation
-- the rights to use, copy, modify, merge, publish, distribute, sublicense,
-- and/or sell copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------

smodule 'adbus'
require 'adbuslua_core'
require 'pretty'

-------------------------------------------------------------------------------
-- Connection API
-------------------------------------------------------------------------------

local mt = {}
mt.__index = mt

_M.connect_address = adbuslua_core.connect_address
_M.bind_address = adbuslua_core.bind_address

DEBUG = false

function _M.enable_debug(dbg)
    if dbg == nil or dbg then
        DEBUG = true
        adbuslua_core.set_log_level(2)
    else
        DEBUG = false
        adbuslua_core.set_log_level(0)
    end
end

--- Creates a connection
-- \param bus       The bus to connect to.
-- \param debug     boolean flag to enable/disable debugging (default
--                  disabled)
--
-- \return The new connection object
--
-- The bus can be one of:
-- - nil - we then connect to the default bus ('session')
-- - 'session' or 'system' to connect to the session or system buses
--   respectively
-- - A string giving the bus address in dbus form eg.
--   'tcp:host=localhost,port=12345'

local connections = {}

function _M.connection(bus)
    local self = connections[bus or 'default']
    if self then return self end

    local self = setmetatable({}, mt)

    self._queue             = {}
    self._connection        = adbuslua_core.connection.new(bus, self._queue)
    self._current_message   = nil
    self._return_message    = nil
    self._return_yield      = {}

    self._callback = function()
        self._return_message = self:current_message()
        self._return_yield.yield = true
    end

    connections[bus or 'default'] = self

    return self
end

function mt:close()
    self._connection:close()
end

--- Returns whether we successfully connected to the bus.
function mt:is_connected_to_bus()
    return self._connection:is_connected_to_bus()
end

--- Returns our unique name if we are connected to the bus or nil.
function mt:unique_name()
    return self._connection:unique_name()
end

--- Returns a new serial for use with messages.
function mt:serial()
    return self._connection:serial()
end

function mt:process(yieldtable)
    while not yieldtable.yield do

        -- Process the queue
        local i = 1
        while true do
            local msg = self._queue[i]
            self._queue[i] = nil

            if not msg then 
                break 
            end

            i = i + 1

            if DEBUG then
                print(table.show(msg, "Received"))
            end

            self._current_message = msg

            if msg.callback then

                -- Match or reply callback
                if msg.type == "error" then
                    msg.callback(nil, msg.error, msg[1])
                else
                    msg.callback(unpack(msg))
                end

            elseif msg.object then
                
                -- Method call
                local obj = msg.object
                local retsigs = msg.user
                local method = msg.member

                if msg.no_reply then
                    obj[method](obj, unpack(msg))
                else
                    local ret = {obj[method](obj, unpack(msg))}
                    ret.destination = msg.sender
                    ret.reply_serial = msg.serial
                    if ret[1] == nil and ret[2] ~= nil then
                        -- 1 is nil
                        -- 2 is error name
                        -- 3 is error msg
                        ret[1] = ret[3]
                        ret.error_name = ret[2]
                        ret.type = "error"
                        ret.signature = ret[1] and "s" or ""
                    else
                        ret.type = "method_return"
                        ret.signature = retsigs[method]
                    end
                    self:send(ret)
                end

            end

            self._current_message = nil

            if yieldtable.yield then
                return
            end
        end

        -- Get more data
        self._connection:process()
    end
end

function mt:current_message()
    return self._current_message
end

--- Adds a match rule to the connection
--
-- The param is a table detailing the fields of messages to match against.
-- Only the callback is required. Optional fields not specified will always
-- match.
--
-- Arguments:
-- \param m.callback            Callback for when the match is hit
-- \param[opt] m.type           type of message. Valid values are:
--                               - 'method_call'
--                               - 'method_return'
--                               - 'error'
--                               - 'signal'
-- \param[opt] m.sender         Service name
-- \param[opt] m.destination    Service name
-- \param[opt] m.interface      String
-- \param[opt] m.reply_serial   Number
-- \param[opt] m.path           String
-- \param[opt] m.member         String
-- \param[opt] m.error          String
-- \param[opt] m.remove_on_first_match      Boolean
-- \param[opt] m.add_to_bus     Boolean
-- \param[opt] m.object         Anything
-- \param[opt] m.id             Number - should be an id returned from
--                              connection:match_id.
-- \param[opt] m.unpack_message Boolean
-- \return The match id. This will be the value of m.id if it is set.
--
-- Details:
-- The sender and destination fields should be valid dbus names (ascii dot
-- seperated string). They can either be a unique name (begins with : eg
-- :1.34) or a service name (reverse dns eg org.freedesktop.DBus) in which
-- case the service will be tracked.
--
-- The path should be a valid dbus path name (unix style path eg
-- /org/freedesktop/DBus).
--
-- The id can be filled out with a return number from connection:match_id
-- or it can be left blank in which case an id will be generated. Either way
-- the id is returned and should be used as the paramater to remove_match.
--
-- By default the match will be kept active until the match is explicitely
-- removed or the connection is destroyed. Alternatively setting
-- m.remove_on_first_match will remove the match the first time it hits a
-- match. This is most often used to match return values.
--
-- The match rule will normally be a local only match and thus can match
-- method_calls (method_calls should really be redirected via an interface),
-- method_returns and errors, but in order to match a signal the match rule
-- must be added to the bus daemon by setting m.add_to_bus.
-- Otherwise the bus will not redirect the signal message our way. This is not
-- necessary for non bus connection.
--
-- The arguments to the callback depend on two params (unpack_message and
-- object). If object is set then the first argument to the function will be
-- that object. If unpack_message is set to true or not set (ie default), the
-- message arguments will be unpacked so the call to the callback will be of
-- the form:
--
-- callback(object, arg0, arg1, ...) or callback(arg0, arg1, ...)
--
-- unpacked error message take a special form. In this case the callback is
-- callback is called with the object (optional), a nil, the message error
-- name, and then the message description (if provided). eg:
--
-- callback(object, nil, 'org.freedesktop.DBus.Error.UnknownMethod',
--          'org.freedesktop.DBus does not understand message foo')
--
-- If unpack_message is set to false then the second argument (if object is
-- set) will be a message object. See connection.send for the format of 
-- this object. eg
-- 
-- callback(object, message) or callback(message)
--
-- Example:
-- \code
-- local function owner_changed(name, from, to)
-- end
--
-- c:add_match{
--      type = 'signal',
--      add_to_bus = true,
--      sender = 'org.freedesktop.DBus',
--      member = NameOwnerChanged
-- }
-- \endcode
--          
--
-- \sa  connection:remove_match
--      connection:send_messsage
--
function mt:add_match(m)
    if DEBUG then
        print(table.show(m, "Adding match"))
    end

    return self._connection:add_match(m.callback, m)
end

function mt:add_reply(remote, serial, callback)
    if DEBUG then
        print(table.show(m, "Adding reply"))
    end

    return self._connection:add_reply(remote, serial, callback)
end

local bind = {}
bind.__index = bind

function bind:emit(member, ...)
    local sig = self._interface._signal_signature[member]
    if not sig then
        error("Invalid signal name")
    end

    local msg = {
        type        = 'signal',
        path        = self._path,
        interface   = self._interface.name,
        signature   = sig,
        member      = member,
        ...
    }

    self._connection:send(msg)
end

function mt:bind(object, path, interface)
    if type(interface) == "string" then
        interface = _M.interface.new(interface)
    end

    local user      = interface._method_return_signature
    local b         = setmetatable({}, bind)
    b._bind         = self._connection:bind(object, user, path, interface._interface)
    b._interface    = interface
    b._path         = path
    b._connection   = self
    return b
end


--- Sends a new message
-- 
-- If arguments are provided, then the signature must be set. The format of
-- the signature can be either:
-- - A single string of the dbus signatures of each argument concatanated
--   together
-- - A table of strings of dbus signatures for each argument
--
-- The dbus signature is an ascii string that represents the marshalled type.
-- Valid values for this are:
--
-- The simple types are:
-- y    - 8 bit unsigned integer
-- b    - Boolean value
-- n    - 16 bit signed integer
-- q    - 16 bit unsigned integer
-- i    - 32 bit signed integer
-- u    - 32 bit unsigned integer
-- x    - 64 bit signed integer
-- t    - 64 bit unsigned integer
-- d    - 64 bit floating point number (double)
-- s    - string
-- o    - dbus object path
-- g    - dbus signature
-- 
-- The compound types are (using X as a placeholder for the inner type):
-- aX    - an array of values of type X
--       - X must be a single conceptual type
-- (X)   - a structure of values of type X 
--       - X can be one or more types in sequence
-- v     - a variant where each occurance dictates the type with the data
-- a{XY} - a dictionary/table/map with keys of type X and values of type Y
--
-- The marshalling from lua values can be grouped into a number of categories:
-- - numeric dbus values (8, 16, 32, 64 bit integers and double) expect a lua
--   number argument
-- - boolean dbus values expect a lua boolean argument
-- - string dbus values (string, object path, and signature) expect a lua
--   string
-- - dbus arrays expect a table of the appropriate values of length #table
--   using integer indices starting at 1 (standard lua arrays)
-- - dbus maps expect a table with the appropriate key and value types
-- - dbus variants will marshall in a number of ways
--   - a lua number is marshalled as a double
--   - a lua boolean is marshalled as a boolean
--   - a lua string is marshalled as a string
--   - a lua table is marshalled as an array of variants if table[1] ~= nil 
--     or as a dbus map with keys and values as variants
--   - if the value
--
--
-- The argument is a message object. The same format is also used for packed
-- match callbacks. This is a table with a number of fields:
-- \param m.type                The type of message. This can be one of:
--                               - 'method_call'
--                               - 'method_return'
--                               - 'error'
--                               - 'signal'
-- \param[opt] m.serial         The serial of the message. If set this should
--                              be set using a return of connection.next_serial.
-- \param[opt] m.interface      Interface field
-- \param[opt] m.path           Path field
-- \param[opt] m.member         Member field
-- \param[opt] m.error          error name field
-- \param[opt] m.destination    Destination field
-- \param[opt] m.sender         Sender field
-- \param[opt] m.signature      Signature of the message. See above.
-- \param[opt] m.no_reply       If set to true, sets the NO_REPLY_EXPECTED
--                              flag in the message. This is an optimisation
--                              where the remote side can then choose to not
--                              send back a reply.
-- \param[opt] m.no_auto_start  If set to true, sets the NO_AUTO_START flag in
--                              the message. This is an indication to the bus
--                              to not auto start a daemon to handle the
--                              message.
-- \param[opt] m[1], m[2] ...   The arguments of the message. They are
--                              marshalled using the signature field.
--
-- \sa  connection.add_match 
--      http://dbus.freedesktop.org/doc/dbus-specification.html
--
function mt:send(m)
    if DEBUG then
        print(table.show(m, "Sending"))
    end

    self._connection:send(m)
end

-- Method callers

function mt:async_call(msg)
    local serial = self:serial()

    local reply
    if msg.callback then
        reply = self:add_reply(msg.destination, serial, msg.callback)
    end

    msg.type = "method_call"
    msg.serial = serial
    msg.callback = nil

    self:send(msg)

    return reply
end

function mt:call(msg)
    if not msg.no_reply then
        msg.callback = self._callback
    end

    local reply = self:async_call(msg)
    msg._callback = nil

    if reply then
        self._return_yield.yield = false
        self._return_message = nil
        self:process(self._return_yield)
        
        local ret = self._return_message
        if ret.type == "error" then
            return nil, ret.error, ret[1]
        else
            return unpack(ret)
        end
    end
end


