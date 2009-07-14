#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

local table         = require("table")
local string        = require("string")
local xml           = require("xml")
local setmetatable  = _G.setmetatable
local error         = _G.error
local unpack        = _G.unpack
local ipairs        = _G.ipairs
local print         = _G.print

module("adbuslua")

proxy = {}
proxy.__index = proxy

function proxy.new(connection, reg)
    if reg.interface == nil or reg.path == nil or reg.service == nil then
        error("Proxies require an explicit service, path, and interface")
    end

    local self = {}
    setmetatable(self, proxy)

    self._interface = reg.interface
    self._path = reg.path
    self._service = reg.service
    self._connection = connection
    local serial = connection:next_serial()
    connection:send_message{
        type        = "method_call",
        serial      = serial,
        destination = reg.service,
        path        = reg.path,
        interface   = "org.freedesktop.DBus.Introspectable",
        member      = "Introspect",
    }

    local match = connection:add_match{
        type            = "method_return",
        reply_serial    = serial,
        unpack_message  = true,
        callback        = {self._introspection_callback, self},
        remove_on_first_match = true,
    }

    connection:process_until_match(match)

    return self
end

function proxy:_call_method(name, signature, ...)
    local serial = self._connection:next_serial()

    self._connection:send_message{
        type        = "method_call",
        destination = self._service,
        path        = self._path,
        interface   = self._interface,
        member      = name,
        signature   = signature,
        serial      = serial,
        ...
    }

    local match = self._connection:add_match{
        type            = "method_return",
        reply_serial    = serial,
        unpack_message  = false,
        callback        = {self._method_callback, self},
        remove_on_first_match = true,
    }

    self._connection:process_until_match(match)

    local ret = self._method_return_message
    self._method_return_message = nil

    -- Note this will drop the named fields of the message
    return unpack(ret)
end

function proxy:_method_callback(message)
    self._method_return_message = message
end

function proxy:_process_method(member)
    local name = member.name
    local signature = {}

    if name:sub(1,1) == '_' then
        return
    end

    for _,arg in ipairs(member) do
        if arg.direction == nil or arg.direction == "in" then
            table.insert(signature, arg.type)
        end
    end

    local fullsig = ""
    for _,v in ipairs(signature) do fullsig = fullsig .. v end

    print("Adding", name, fullsig)

    self[name] = function(self, ...)
        return self:_call_method(name, signature, ...)
    end
end

function proxy:_process_signal(member)
    -- TODO
end

function proxy:_process_property(member)
    -- TODO
end

function proxy:_introspection_callback(introspection)
    self._signatures = {}

    x = xml.eval(introspection)

    for _,interface in ipairs(x) do
        if interface.name == self._interface then
            print(string.format("Got introspection for %s", interface.name))
            for _,member in ipairs(interface) do
                local tag = member:tag()
                if tag == "method" then
                    self:_process_method(member)
                elseif tag == "signal" then
                    self:_process_signal(member)
                elseif tag == "property" then
                    self:_process_property(member)
                end
            end
            break
        end
    end

end

