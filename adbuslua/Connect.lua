#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

local math          = require("math")
local os            = require("os")
local io            = require("io")
local sha1          = require("sha1")
local socket        = require("socket")
local adbuslua_core = require("adbuslua_core")
local string        = require("string")
local tostring      = _G.tostring
local tonumber      = _G.tonumber
local print         = _G.print
local assert        = _G.assert

module("adbuslua")

math.randomseed(os.time())

getlocalid = adbuslua_core.getlocalid

function _hex_encode(str)
    local ret = ""
    for i = 1, str:len() do
        ret = ret .. string.format("%02x", str:byte(i))
    end
    return ret
end

function _hex_decode(str)
    local ret = ""
    for i = 1, str:len(), 2 do
        local val = tonumber(str:sub(i,i+1), 16)
        local char = string.format("%c", val)
        ret = ret .. char
    end
    return ret
end

local function random_string(length)
    local ret = ""
    for i = 1, length do
        local val = math.random(0, 255)
        local char = string.format("%c", val)
        ret = ret .. char
    end
    return ret
end

function _printf(formatstr, ...)
    print(string.format(formatstr, ...))
end

function connect_dbus_cookie_sha1(host, port, id)

    local sock = socket.connect(host, port)
    assert(sock, string.format("Failed to connect to %s:%s", host, port))
    sock:send("\0")

    local sendauthline = "AUTH DBUS_COOKIE_SHA1 " .. _hex_encode(tostring(id)) .. "\r\n"
    sock:send(sendauthline)

    local authline = sock:receive("*l")
    local encoded = authline:match("DATA (%x+)")
    assert(encoded)
    local keyring, id, servdata = _hex_decode(encoded):match("([%w_]+) (%w+) (%x+)")

    local keyringfile = io.open(os.getenv("HOME") .. "/.dbus-keyrings/" .. keyring, "r")
    local keyringdata
    for line in keyringfile:lines() do
        local lineid
        lineid, _, keyringdata = line:match("(%w+) (%w+) (%w+)")
        if lineid == id then
            break
        end
    end

    local localdata = _hex_encode(random_string(32))
    local stringtosha = servdata .. ":" .. localdata .. ":" .. keyringdata
    local sha = _hex_encode(sha1.digest(stringtosha))

    sock:send("DATA " .. _hex_encode(localdata .. " " .. sha) .. "\r\n")

    local okline = sock:receive()
    local guid = okline:match("OK (%x+)")
    assert(guid)

    sock:send("BEGIN\r\n")
    _printf("Connected to tcp:host=%s,port=%d with id %s and bus guid %s", host, port, tostring(id), guid)
    return sock
end

function connect_external(host, port, id)
    local sock = socket.connect(host, port)
    sock:send("\0")

    local sendauthline = "AUTH EXTERNAL " .. _hex_encode(tostring(id)) .. "\r\n"
    sock:send(sendauthline)

    local okline = sock:receive()
    local guid = okline:match("OK (%x+)")
    assert(guid)

    sock:send("BEGIN\r\n")
    _printf("Connected to tcp:host=%s,port=%d with id %s and bus guid %s", host, port, tostring(id), guid)
    return sock
end

local function sock_send(sock, sendbuf, data)
end

local function sock_receive(sock, sendbuf)
end

function new_tcp_connection(host, port)
    local sock = adbus.socket.new(host, port)
    local win32 = getlocalid():sub(1,1) == "S"
    local sock
    if win32 then
        sock = connect_external(host, port, getlocalid())
    else
        sock = connect_dbus_cookie_sha1(host, port, getlocalid())
    end

    local sendbuf = {}
    sock:settimeout(0)

    local conn = connection.new(function(data)
        sock_send(sock, sendbuf, data) 
    end)

    conn:set_send_callback(function()
        return sock_receive(sock, sendbuf)
    end)

    return conn
end

adbus.connection.new(adbus.new_tcp_socket("localhost", 12345))

sock:receive()
sock:send("foo")


