#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

local adbuslua_core = require("adbuslua_core")
local string        = require("string")
local socket        = require("socket")
local print         = _G.print
local unpack        = _G.unpack
local setmetatable  = _G.setmetatable

module("adbus")

interface     = adbuslua_core.interface
object        = adbuslua_core.object
send_reply    = adbuslua_core.send_reply
send_error    = adbuslua_core.send_error

connection = {}
connection.__index = connection




function connection.new(socket)
    local self = {}
    setmetatable(self, connection)
    self._socket = socket
    self._connection = adbuslua_core.connection.new()

    self._connection:set_send_callback(function(data) self:_send(data) end)
    self._socket:settimeout(0)
    self._tosend = ""

    return self
end

function connection:connect_to_bus()
    self._connection:connect_to_bus(function() self:_connected_to_bus() end)
    -- connected callback with pull us out
    self:process_messages()
end

function connection:is_connected_to_bus()
    return self._connection:is_connected_to_bus()
end

function connection:unique_service_name()
    return self._connection:unique_service_name()
end

function connection:next_serial()
    return self._connection:next_serial()
end

function connection:add_object(path)
    return self._connection:add_object(path)
end

function connection:process_messages()
    self._yield = false
    self._yield_match = nil
    self:_process_messages()
end

function connection:process_until_match(match)
    self._yield = false
    self._yield_match = match
    self:_process_messages()
end

function connection:add_match(registration)
    registration.callback = {
        connection._match_callback,
        {self, registration.callback, registration.unpack_message}
    }
    registration.unpack_message = nil

    return self._connection:add_match(registration)
end

function connection:remove_match(match)
    self._connection:remove_match(match)
end

function connection:add_bus_match(registration)
    registration.callback = {
        connection._match_callback,
        {self, registration.callback, registration.unpack_message}
    }
    registration.unpack_message = nil

    return self._connection:add_bus_match(registration)
end

function connection:remove_bus_match(match)
    self._connection:remove_bus_match(match)
end

function connection:new_proxy(registration)
    return proxy.new(self, registration)
end

function connection:send_message(message)
    self._connection:send_message(message)
end








function connection:_connected_to_bus()
    _printf("Connected to bus with service %s",
          self._connection:unique_service_name())
    self._yield = true
end

function connection:_send(data)
    self._tosend = self._tosend .. data
    self:_trysend()
end

function connection:_trysend()
    local _,res,i = self._socket:send(self._tosend)
    if res == "timeout" then
        self._tosend = self._tosend:sub(i)
    else
        self._tosend = ""
    end
end

function connection:_process_messages()
    while not self._yield do
        local to_read = {self._socket}
        local to_write = {}
        if self._tosend:len() > 0 then
            to_write = {self._socket}
        end
        local ready_read, ready_write, err = socket.select(to_read, to_write)

        if err ~= nil then
            print ("Error", err)
            break
        end

        if ready_read[1] == self._socket then
            local data,err,partial = self._socket:receive(4096)
            if err ~= nil and err ~= "timeout" then
                print("Error", err)
                break
            elseif data ~= nil then
                self._connection:parse(data)
            elseif partial ~= nil then
                self._connection:parse(partial)
            end
        end

        if ready_write[1] == self._socket and self._tosend:len() > 0 then
            self:_trysend()
        end

    end
end

function connection._match_callback(data, message, match)
    local self      = data[1]
    local callback  = data[2]
    local unpack_msg = data[3]
    local func      = callback[1]
    local obj       = callback[2]
    if unpack_msg then
        func(obj, unpack(message))
    else
        func(obj, message, match)
    end
    if self._yield_match == match then
        self._yield = true
    end
end

