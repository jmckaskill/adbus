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


smodule 'adbus'
require 'xml'
require 'table'
require 'string'

-------------------------------------------------------------------------------
-- Proxy API
-------------------------------------------------------------------------------

_M.proxy = _M.proxy or {}
proxy = _M.proxy

local proxy_mt = {}
local call
local process_introspection

--- Gets the connection object associated with the proxy
function proxy.connection(self)
    return rawget(self, "_p").connection
end

--- Gets the path of the remote object
function proxy.path(self)
    return rawget(self, "_p").path
end

--- Gets the name of the remote
function proxy.service(self)
    return rawget(self, "_p").service
end

--- Gets the interface associated with the proxy if it was specified
--- originally
function proxy.interface(self)
    return rawget(self, "_p").interface
end

function proxy.node(self, path)
    local c = proxy.connection(self)
    local s = proxy.service(self)
    local p = proxy.path(self)
    if p ~= '/' then p = p .. '/' end
    return c:proxy(s, p .. path)
end

local function lassert(nil_check, name, msg, ...)
    if nil_check == nil and name then
        return assert(nil, name .. ":\n" .. (msg and (msg .. "\n") or ''))
    else
        return nil_check, name, msg, ...
    end
end

function proxy.call(self, t)
    local c = proxy.connection(self)
    t.path = proxy.path(self)
    t.destination = proxy.service(self)
    t.interface = t.interface or proxy.interface(self)
    if t.reply == nil or t.reply then
        return lassert(c:call(t))
    else
        c:call(t)
    end
end

function proxy.member_type(self, member)
    if type(rawget(self, member)) == "function" then
        return "method"
    elseif rawget(self, "_p").signals[member] then
        return "signal"
    elseif rawget(self, "_p").get_properties[member] then
        return "property"
    elseif rawget(self, "_p").set_properties[member] then
        return "property"
    else
        return "unknown"
    end
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
-- \return  match which can be used with proxy.disconnect to disconnect the
--          signal
--
-- The callback function's arguments are the same as for the
-- connection.add_match's callback.
function proxy.connect(self, t)
    local matches    = rawget(self, "_p").signal_matches
    local connection = proxy.connection(self)
    local sigs       = rawget(self, "_p").signals
    local interface  = sigs[t.member]
    assert(interface, "Invalid signal name")

    local match = {
        add_match_to_bus_daemon = true,
        type            = "signal",
        path            = proxy.path(self),
        sender          = proxy.service(self),
        interface       = interface,
        unpack_message  = t.unpack_message,
        object          = t.object,
        callback        = t.callback,
    }

    local m = connection:add_match(match)

    matches[m] = true
    return m
end

--- Disconnects one or all signals
-- 
-- \param       self        The proxy object
-- \param[opt]  match       The match returned from a call to proxy.connect
--
-- If the match is not specified this will disconnect all signals.
function proxy.disconnect(self, match)
    local connection = proxy.connection(self)
    if not match then
        local p = rawget(self, "_p")
        for match in pairs(p.signal_matches) do
            match:close()
        end
        p.signal_matches = {}
    else
        match:close()
        p.signal_matches[match] = nil
    end
end

-- Properties

function proxy_mt:__index(key)
    local props = rawget(self, "_p").get_properties
    local prop  = props[key]
    assert(prop, "Invalid property name")

    local interface = prop[2]

    return self:_call{
        interface   = 'org.freedesktop.DBus.Properties',
        member      = 'Get',
        signature   = "ss",
        interface,
        key,
    }
end

function proxy_mt:__newindex(key, value)
    local props = rawget(self, "_p").set_properties
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

    return self:_call{
        interface   = 'org.freedesktop.DBus.Properties',
        member      = 'Set',
        signature   = "ssv",
        interface,
        key,
        data,
    }
end

function proxy_mt:__tostring()
    return proxy.help(self)
end

-------------------------------------------------------------------------------
-- Private API
-------------------------------------------------------------------------------

-- Constructor called from connection.proxy
function proxy._new(connection, service, path, interface)
    if path == nil or service == nil then
        error("Proxies require an explicit service and path")
    end

    local self = {}
    setmetatable(self, proxy_mt)

    local p = {}
    p.get_properties    = {}
    p.set_properties    = {}
    p.signals           = {}
    p.signal_matches    = {}
    p.nodes             = {}
    p.interface         = interface
    p.path              = path
    p.service           = service
    p.connection        = connection
    rawset(self, "_p", p)

    rawset(self, "_introspect", function()
        return self:_call{
            interface   = 'org.freedesktop.DBus.Introspectable',
            member      = 'Introspect',
        }
    end)

    rawset(self, "_help", proxy.help)
    rawset(self, "_path", proxy.path)
    rawset(self, "_connection", proxy.connection)
    rawset(self, "_service", proxy.service)
    rawset(self, "_connect", proxy.connect)
    rawset(self, "_disconnect", proxy.disconnect)
    rawset(self, "_call", proxy.call)
    rawset(self, "_node", proxy.node)
    rawset(self, "_member_type", proxy.member_type)

    process_introspection(self, self:_introspect())

    return self
end

-- Introspection XML processing
local function process_method(self, interface, member)
    local signature = ""

    for _,arg in ipairs(member) do
        if arg.direction == nil or arg.direction == "in" then
            signature = signature .. arg.type
        end
    end

    rawset(self, member.name, function(self, ...)
        local t = {
            member = member.name,
            interface = interface,
            signature = signature,
            ...
        }
        return self:_call(t)
    end)
end

local function process_signal(self, interface, member)
    local name = member.name
    local sigs = rawget(self, "_p").signals

    sigs[name] = interface
end

local function process_property(self, interface, member)
    local name      = member.name
    local signature = member.type
    local access    = member.access

    if access == "read" or access == "readwrite" then
        local get_props = rawget(self, "_p").get_properties
        get_props[name] = {signature, interface}
    end

    if access == "write" or access == "readwrite" then
        local set_props = rawget(self, "_p").set_properties
        set_props[name] = {signature, interface}
    end
end

local function process_node(self, node)
    local c = proxy.connection(self)
    local nodes = rawget(self, "_p").nodes
    local path = proxy.path(self)
    nodes[node.name] = true
end


process_introspection = function(self, introspection)
    local x         = xml.eval(introspection)
    local interface = proxy.interface(self)

    for _,node in ipairs(x) do
        if node:tag() == "interface" and (interface == nil or node.name == interface) then
            for _,member in ipairs(node) do
                local tag = member:tag()
                if tag == "method" then
                    process_method(self, node.name, member)
                elseif tag == "signal" then
                    process_signal(self, node.name, member)
                elseif tag == "property" then
                    process_property(self, node.name, member)
                end
            end
        elseif node:tag() == "node" then
            process_node(self, node)
        end
    end
end


