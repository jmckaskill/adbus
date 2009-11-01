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
--
-------------------------------------------------------------------------------

local table         = require("table")
local string        = require("string")
local luaxml        = require("luaxml")
local setmetatable  = _G.setmetatable
local error         = _G.error
local unpack        = _G.unpack
local ipairs        = _G.ipairs
local print         = _G.print
local rawget        = _G.rawget
local rawset        = _G.rawset
local assert        = _G.assert

-------------------------------------------------------------------------------
-- Proxy API
-------------------------------------------------------------------------------

module("adbuslua")

proxy = {}
local proxy_mt = {}
local call_method
local process_introspection

--- Gets the connection object associated with the proxy
function proxy.connection(self)
    return rawget(self, "_connection")
end

--- Gets the path of the remote object
function proxy.path(self)
    return rawget(self, "_path")
end

--- Gets the name of the remote
function proxy.service(self)
    return rawget(self, "_service")
end

--- Gets the interface associated with the proxy if it was specified
--- originally
function proxy.interface(self)
    return rawget(self, "_interface")
end

--- Connects a signal
--
-- \param       t.member    Name of the signal
-- \param       t.proxy     Proxy object
-- \param       t.callback  Callback function
-- \param[opt]  t.object    Value to be given as first arg to the callback
-- \param[opt]  t.unpack_message    Whether to unpack the message when calling
--                                  the callback (default true)
--
-- \return  match id which can be used with proxy.disconnect to disconnect the
--          signal
--
-- The callback function's arguments are the same as for the
-- connection.add_match's callback.
function proxy.connect(t)
    local self       = t.proxy
    local match_ids  = rawget(self, "_signal_match_ids");
    local connection = proxy.connection(self)
    local sigs       = rawget(self, "_signals");
    local interface  = sigs[t.member]
    assert(interface, "Invalid signal name")

    local id = connection:add_match{
        add_match_to_bus_daemon = true,
        type            = "signal",
        interface       = interface,
        path            = proxy.path(self),
        sender          = proxy.service(self),
        member          = signal_name,
        callback        = t.callback,
        object          = t.object,
        unpack_message  = t.unpack_message,
    }

    table.insert(match_ids, id)
    return id
end

--- Disconnects one or all signals
-- 
-- \param       self        The proxy object
-- \param[opt]  match_id    The match_id returned from a call to proxy.connect
--
-- If the match_id is not specified this will disconnect all signals.
function proxy.disconnect(self, match_id)
    local connection = proxy.connection(self)
    if match_id == nil then
        for _,id in ipairs(rawget(self, "_signal_match_ids")) do
            connection:remove_match(match_id)
        end
    else
        connection:remove_match(match_id)
    end
end

-- Properties

function proxy_mt:__index(key)
    local props = rawget(self, "_get_properties")
    local prop  = props[key]
    assert(prop, "Invalid property name")

    local interface = prop[2]

    return call_method(self,
                       "org.freedesktop.DBus.Properties",
                       "Get",
                       {"s", "s"},
                       interface,
                       key)
end

function proxy_mt:__newindex(key, value)
    local props = rawget(self, "_set_properties")
    local prop  = props[key]
    assert(prop, "Invalid property name")

    local signature = prop[1]
    local interface = prop[2]
    local data = {
        __dbus_signature = signature,
        __dbus_value = value,
    }
    setmetatable(data, data)

    if DEBUG then
        print(table.show(value, proxy.path(self) .. "." .. key))
    end

    return call_method(self,
                       "org.freedesktop.DBus.Properties",
                       "Set",
                       {"s", "s", "v"},
                       interface,
                       key,
                       data)
end

-------------------------------------------------------------------------------
-- Private API
-------------------------------------------------------------------------------

-- Constructor called from connection.proxy
function proxy._new(connection, reg)
    if reg.path == nil or reg.service == nil then
        error("Proxies require an explicit service and path")
    end

    local self = {}
    setmetatable(self, proxy_mt)

    rawset(self, "_get_properties", {})
    rawset(self, "_set_properties", {})
    rawset(self, "_signals", {})
    rawset(self, "_signal_match_ids", {})
    rawset(self, "_interface", reg.interface)
    rawset(self, "_path", reg.path)
    rawset(self, "_service", reg.service)
    rawset(self, "_connection", connection)

    local xml = call_method(self,
                            "org.freedesktop.DBus.Introspectable",
                            "Introspect")

    process_introspection(self, xml)

    return self
end


-- Method callers

local function method_callback(self, message)
    rawset(self, "_method_return_message", message)
end

call_method = function(self, interface, name, signature, ...)
    local connection = proxy.connection(self)
    local serial = connection:next_serial()

    local message = {
        type        = "method_call",
        destination = proxy.service(self),
        path        = proxy.path(self),
        interface   = interface,
        member      = name,
        signature   = signature,
        serial      = serial,
        ...
    }

    connection:send_message(message)

    local match = connection:add_match{
        reply_serial    = serial,
        unpack_message  = false,
        callback        = method_callback,
        object          = self,
        remove_on_first_match = true,
    }

    connection:process_until_match(match)

    local ret = rawget(self, "_method_return_message")
    rawset(self, "_method_return_message", nil)

    if ret.type == "error" then
        assert(nil, ret.error_name .. ": " .. ret[1])
    else
        return unpack(ret)
    end
end

-- Introspection XML processing

local function process_method(self, interface, member)
    local name = member.name
    local signature = {}

    for _,arg in ipairs(member) do
        if arg.direction == nil or arg.direction == "in" then
            table.insert(signature, arg.type)
        end
    end

    rawset(self, name, function(proxy, ...)
        return call_method(self, interface, name, signature, ...)
    end)
end

local function process_signal(self, interface, member)
    local name = member.name
    local sigs = rawget(self, "_signals")

    sigs[name] = interface
end

local function process_property(self, interface, member)
    local name      = member.name
    local signature = member.type
    local access    = member.access

    if access == "read" or access == "readwrite" then
        local get_props = rawget(self, "_get_properties")
        get_props[name] = {signature, interface}
    end

    if access == "write" or access == "readwrite" then
        local set_props = rawget(self, "_set_properties")
        set_props[name] = {signature, interface}
    end
end


process_introspection = function(self, introspection)
    local x         = luaxml.eval(introspection)
    local interface = proxy.interface(self)

    for _,xml_interface in ipairs(x) do
        if interface == nil or xml_interface.name == interface then
            for _,xml_member in ipairs(xml_interface) do
                local tag = xml_member:tag()
                if tag == "method" then
                    process_method(self, xml_interface.name, xml_member)
                elseif tag == "signal" then
                    process_signal(self, xml_interface.name, xml_member)
                elseif tag == "property" then
                    process_property(self, xml_interface.name, xml_member)
                end
            end
        end
    end
end


