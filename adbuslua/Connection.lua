#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

local adbuslua_core = require("adbuslua_core")
local string        = require("string")
local table         = require("table")
local os            = require("os")
local print         = _G.print
local unpack        = _G.unpack
local setmetatable  = _G.setmetatable
local assert        = _G.assert
local error         = _G.error

module("adbuslua")

interface     = adbuslua_core.interface

connection = {}
connection.__index = connection

local function parse_address(address, options, backup)
    local bus = os.getenv(address) or backup
    assert(bus, string.format("%s is not set", address))
    local type,opts = bus:match("([^:]*):([^:]*)")
    options.type = type
    for k,v in opts:gmatch("([^=,]*)=([^=,]*)") do
        options[k] = v
    end
end

function connection.new(options)
    local socket = options.socket
    if socket == nil then
        if options.type == "session" then
            parse_address("DBUS_SESSION_BUS_ADDRESS", options)
        elseif options.type == "system" then
            parse_address("DBUS_SYSTEM_BUS_ADDRESS", options, "unix:file=/var/run/dbus/system_bus_socket")
        end

        if options.type == "tcp" and options.host and options.port then
            socket = adbuslua_core.socket.new_tcp(options.host, options.port)
        elseif options.type == "unix" and options.abstract ~= nil then
            socket = adbuslua_core.socket.new_unix(options.abstract, true)
        elseif options.type == "unix" and options.file ~= nil then
            socket = adbuslua_core.socket.new_unix(options.file, false)
        else
            error("Don't know how to create socket")
        end
    end

    local self = {}
    setmetatable(self, connection)

    self._connection = adbuslua_core.connection.new(options.debug)
    self._socket     = socket;

    self._connection:set_send_callback(function(data)
        self._socket:send(data)
    end)

    self._connection:connect_to_bus()

    return self
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

function connection:process_messages()
    self._yield = false
    self._yield_match = nil
    while not self._yield do
        local data = self._socket:receive()
        self._connection:parse(data)
    end
end

function connection:process_until_match(match)
    self._yield = false
    self._yield_match = match
    while not self._yield do
        local data = self._socket:receive()
        self._connection:parse(data)
    end
end

function connection:_check_yield(id)
    if id == self._yield_match then
        self._yield = true
    end
end

function connection:add_match(reg)
    local id = self._connection:next_match_id()
    local func = reg.callback
    reg.id = id

    if reg.unpack_message or reg.unpack_message == nil then
        reg.callback = function(object, message)
            if message.type == "error" then
                func(object, nil, message.error_name, unpack(message))
            else
                func(object, unpack(message))
            end

            self._check_yield(id)
        end

    else
        reg.callback = function(object, message)
            func(object, message)
            self:_check_yield(id)
        end

    end

    -- Get rid of unpack_message as the underlying c library doesn't want to
    -- know about it
    reg.unpack_message = nil

    self._connection:add_match(reg)
    return id
end


function connection:remove_match(match)
    self._connection:remove_match(match)
end

function connection:new_proxy(reg)
    return proxy.new(self, reg)
end

function connection:send_message(message)
    self._connection:send_message(message)
end






--[[
        local to_read = {self._socket}
        local to_write = {}
        if self._tosend:len() > 0 then
            to_write = {self._socket}
        end
        local ready_read, ready_write, err = socket.select(to_read, to_write)

        if err ~= nil then
            assert(nil, err)
        end

        if ready_read[1] == self._socket then
            local alldata,data,err,partial
            -- loop until we hit a timeout
            while err == nil do
                data,err,partial = self._socket:receive(4096)
                if data ~= nil then
                    alldata = (alldata or "") .. data
                elseif partial ~= nil then
                    alldata = (alldata or "") .. partial
                end
            end
            if alldata ~= nil then
                --_printf("Received %s", hex_dump(alldata, "         "))
                self._connection:parse(alldata)
            end
            if err ~= "timeout" then
                assert(nil, err)
            end
        end

        if ready_write[1] == self._socket and self._tosend:len() > 0 then
            self:_trysend()
        end

    end
end
--]]

