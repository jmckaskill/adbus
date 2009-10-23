#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

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

module("adbuslua")

proxy = {}
local proxy_mt = {}

-- Method callers

local function method_callback(self, message)
    rawset(self, "_method_return_message", message)
end

local function call_method(self, interface, name, signature, ...)
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


local function process_introspection(self, introspection)
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

-- Constructor

function proxy.new(connection, reg)
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

-- Data accessors

function proxy.connection(self)
    return rawget(self, "_connection")
end

function proxy.path(self)
    return rawget(self, "_path")
end

function proxy.service(self)
    return rawget(self, "_service")
end

function proxy.interface(self)
    return rawget(self, "_interface")
end

-- Signals

function proxy.connect(self, signal_name, callback, object)
    local match_ids  = rawget(self, "_signal_match_ids");
    local connection = proxy.connection(self)
    local sigs       = rawget(self, "_signals");
    local interface  = sigs[signal_name]
    assert(sig, "Invalid signal name")

    local id = connection:add_match{
        add_match_to_bus_daemon = true,
        type        = "signal",
        interface   = sig,
        path        = proxy.path(self),
        source      = proxy.service(self),
        member      = signal_name,
        callback    = callback,
        object      = object,
    }

    table.insert(match_ids)
    return id
end

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

local function printf(...) print(string.format(...)) end

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
