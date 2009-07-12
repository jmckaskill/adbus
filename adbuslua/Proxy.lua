#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

module "adbus"

require "LuaXml"

adbus.proxy = {}
adbus.proxy.__index = adbus.proxy

function adbus.proxy.new(connection, reg)
    if reg.interface == nil or reg.path == nil or reg.service == nil then
        error("Proxies require an explicit service, path, and interface")
    end

    local self = {}
    setmetatable(self, adbus.proxy)

    self._interface = interface
    self._path = path
    self._service = service
    self._connection = connection
    local serial = connection:next_serial()
    connection:send_message{
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

function adbus.proxy:_call_method(name, signature, ...)
    local serial = self._connection:next_serial()
    self._connection:send_message{
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
        callback        = {self._method_callback, self}
        remove_on_first_match = true,
    }

    self._connection:process_until_match(match)

    local ret = self._method_return_message
    self._method_return_message = nil

    -- Note this will drop the named fields of the message
    return unpack(ret)
end

function adbus.proxy:_method_callback(message)
    self._method_return_message = message
end

function adbus.proxy:_process_method(member)
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

    self[name] = function(self, ...)
        return self:_call_method(name, signature, ...)
    end
end

function adbus.proxy:_process_signal(member)
    -- TODO
end

function adbus.proxy:_process_property(member)
    -- TODO
end

function adbus.proxy:_introspection_callback(introspection)
    self._signatures = {}

    x = xml.eval(introspection)

    for _,interface in ipairs(x) do
        if interface.name == self._interface then
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


local proxy = connection:new_proxy{
    service     = "org.freedesktop.DBus",
    path        = "/foo/bar",
    interface   = "some.foo.bar"}

local ret1, ret2 = proxy:Echo("test")
