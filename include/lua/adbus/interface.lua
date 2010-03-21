#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et


smodule 'adbus'
require 'adbuslua_core'


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

function _M.interface(registration)
    local self = {}

    self._signal_signature = {}

    for _,reg in ipairs(registration) do
        if reg[1] == "signal" then
            local sig = ""
            for _,arg in ipairs(reg.arguments) do
                sig = sig .. (arg[2] or arg[1])
            end
            self._signal_signature[reg[2]] = sig

        end
    end

    self._interface = adbuslua_core.interface.new(registration)
    self.name = registration.name
    return self
end



