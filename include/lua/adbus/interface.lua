#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et


local adbuslua_core = require("adbuslua_core")
local unpack        = _G.unpack
local ipairs        = _G.ipairs


module("adbus")

interface = {}
interface.__index = interface

-- for methods:
-- return_args reg.callback(object, args...)
--
-- for methods with not reg.unpack_message:
-- return_args reg.callback(object, message)
--
-- for properties:
-- get: value reg.get_callback(object, message)
-- set: reg.set_callback(object, value, message)
--
-- callbacks can return an error via nil, error_msg, error_name

function interface.new(name, registration)
    local self = {}
    setmetatable(self, interface)

    self._signal_signature = {}

    for _,reg in ipairs(registration) do
        if reg.type == "method" then
            if reg.unpack_message or reg.unpack_message == nil then
                reg.callback = function(object, message) 
                    return reg.callback(object, unpack(message)) 
                end
            end

            reg.unpack_message = nil

        elseif reg.type == "signal" then
            local sig = {}
            for _,arg in ipairs(reg.arguments) do
                table.insert(sig, arg.type)
            end
            self._signal_signature[reg.name] = sig

        end
    end

    self._interface = adbuslua_core.interface.new(name, registration)
    self.name = name
    return self
end



