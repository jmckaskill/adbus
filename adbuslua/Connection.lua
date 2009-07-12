#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

module "adbus"

require "adbuslua_core"

adbus.interface = adbuslua_core.interface
adbus.connection = {}
adbus.connection.__index = adbus.connection




function adbus.connection.new(socket)
    local self = {}
    setmetatable(self, adbus.connection)
    self._process_thread = coroutine.create(self._process_messages)
    self._socket = socket
    self._connection = adbuslua_core.connection.new()

    self._connection:set_send_callback(function(data) self:_send(data) end)
    self._socket:settimeout(0)

    -- Start and initialise the process thread
    coroutine.resume(self._process_thread, self)

    return self
end

function adbus.connection:connect_to_bus()
    self._connection:connect_to_bus()
end

function adbus.connection:add_object(path)
    return self._connection:add_object(path)
end

function adbus.connection:process_messages()
    self._yield = false
    self._yield_match = nil
    coroutine.resume(self._process_thread)
end

function adbus.connection:process_until_match(match)
    self._yield = false
    self._yield_match = match
    coroutine.resume(self._process_thread)
end

function adbus.connection:add_match(registration)
    registration.callback =  {adbus.connection._watch_callback, self} 
                          .. registration.callback

    return self._connection:add_match(registration)
end

function adbus.connection:add_bus_match(registration)
    registration.callback =  {adbus.connection._watch_callback, self} 
                          .. registration.callback

    return self._connection:add_bus_match(registration)
end

function adbus.connection:new_proxy(registration)
    return adbus.proxy.new(self, registration)
end







function adbus.connection:_send(data)
    self._tosend = self._tosend .. data
    self:_trysend()
end

function adbus.connection:_trysend()
    local _,res,i = self._socket:send(self._tosend)
    if res == "timeout" then
        self._tosend = self._tosend:sub(i)
    else
        self._tosend = ""
    end
end
  
function adbus.connection:_process_thread()
    while true do
        coroutine.yield()
        self:_process_messages()
    end
end

function adbus.connection:_process_messages()
    while not self._yield do
        local to_read = {self._socket}
        local to_write = {}
        if tosend:len() > 0 then
            to_write = {self._socket}
        end
        local ready_read, ready_write, err = socket.select(to_read, to_write)

        if err ~= nil then
            print ("Error", err)
            break
        end

        if ready_read[1] == sock then
            local data,err,partial = self._socket:receive(4096)
            if data ~= nil then
                print("received full", hex_encode(data))
                connection:parse(data)
            elseif partial ~= nil then
                print("received partial", hex_encode(partial))
                connection:parse(partial)
            end
        end

        if ready_write[1] == sock and tosend:len() > 0 then
            trysend()
        end

    end
end

function adbus.connection._match_callback(func, object, message, match)
    func(object, message)
    if self._yield_match == match then
        self._yield = true
    end
end

