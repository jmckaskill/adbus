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
object        = adbuslua_core.object

connection = {}
connection.__index = connection

object = {}
object.__index = object
local function dump(str, x) print(table.show(x,str)) end
function connection.new(options)
    local socket = options.socket
    if socket == nil then
        if options.type == "session" then
            local bus = os.getenv("DBUS_SESSION_BUS_ADDRESS")
            dump("bus", bus)
            assert(bus, "DBUS_SESSION_BUS_ADDRESS is not set")
            local type,opts = bus:match("([^:]*):([^:]*)")
            options.type = type
            dump("type", type)
            dump("opts", opts)
            for k,v in opts:gmatch("([^=,]*)=([^=,]*)") do
                options[k] = v
            end
        end
        dump("options", options)

        if options.type == "tcp" then
            socket = adbuslua_core.socket.new_tcp(options.host, options.port)
        elseif options.type == "unix" and options.abstract ~= nil then
            socket = adbuslua_core.socket.new_unix(options.abstract, true)
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

    return self
end

function connection:connect_to_bus()
    self._connection:connect_to_bus(function() self._yield = true end)
    -- connected callback will pull us out
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

function object:__gc()
    self._connection:remove_object(self._path)
end

function object:bind(interface)
    self._connection:bind_interface(self._path, interface)
end

function object:emit(interface, signal_name, ...)
    self._connection:emit(self._path,
                          interface,
                          signal_name,
                          {...},
                          interface._signal_signature(signal_name))
end

function connection:add_object(path)
    self._connection:add_object(path)

    local object = {}
    setmetatable(object, object_mt)
    object._connection = self._connection
    object._path = path
    return object
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

