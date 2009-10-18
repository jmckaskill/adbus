#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

require('adbuslua_core')

module("adbuslua")

object = {}
object.__index = object

local core = adbuslua_core.object

function object.new(connection, path)
    local self = {}
    setmetatable(self, object)

    self.connection = connection
    self.path = path
    self.interfaces = {}
    return self
end


function object:__gc()
    self:unbind_all()
end

function object:bind(interface)
    self.interfaces[interface.name] = interface
    core.bind(self.connection, self.path, interface)
end

function object:unbind(interface)
    if type(interface) == 'string' then
        interface = self.interfaces[interface]
    end
    self.interfaces[interface.name] = nil
    core.unbind(self.connection, self.path, interface)
end

function object:unbind_all()
    for _,interface in pairs(self._interfaces) do
        core.unbind(self.connection, self.path, interface)
    end
end

function object:emit(interface, signal_name, ...)
    core.emit(self.connection,
              self.path,
              interface,
              signal_name,
              interface._signal_signature[signal_name],
              {...})
end


