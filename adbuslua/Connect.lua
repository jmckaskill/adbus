#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et


module "adbus"

require "math"
require "os"
require "io"
require "sha1"
require "socket"
require "adbus"


math.randomseed(os.time())

adbus.getlocalid = adbuslua_core.getlocalid


local function hex_encode(str)
    local ret = ""
    for i = 1, str:len() do
        ret = ret .. string.format("%02x", str:byte(i))
    end
    return ret
end

local function hex_decode(str)
    local ret = ""
    for i = 1, str:len(), 2 do
        local val = tonumber(str:sub(i,i+1), 16)
        local char = string.format("%c", val)
        ret = ret .. char
    end
    return ret
end

local function print_table(table)
    for k,v in pairs(table) do
        print (k,v)
    end
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

local function printf(formatstr, ...)
    print(string.format(formatstr, ...))
end

function adbus.connect_dbus_cookie_sha1(host, port, id)

    local sock = socket.connect(host, port)
    sock:send("\0")

    local sendauthline = "AUTH DBUS_COOKIE_SHA1 " .. hex_encode(tostring(id)) .. "\r\n"
    sock:send(sendauthline)

    local authline = sock:receive("*l")
    local encoded = authline:match("DATA (%x+)")
    assert(encoded)
    local keyring, id, servdata = hex_decode(encoded):match("([%w_]+) (%w+) (%x+)")

    local keyringfile = io.open(os.getenv("HOME") .. "/.dbus-keyrings/" .. keyring, "r")
    local keyringdata
    for line in keyringfile:lines() do
        local lineid
        lineid, _, keyringdata = line:match("(%w+) (%w+) (%w+)")
        if lineid == id then
            break
        end
    end

    local localdata = hex_encode(random_string(32))
    local stringtosha = servdata .. ":" .. localdata .. ":" .. keyringdata
    local sha = hex_encode(sha1.digest(stringtosha))

    sock:send("DATA " .. hex_encode(localdata .. " " .. sha) .. "\r\n")

    local okline = sock:receive()
    local guid = okline:match("OK (%x+)")
    assert(guid)

    sock:send("BEGIN\r\n")
    printf("Connected to tcp:host=%s,port=%d with id %s and bus guid %s", host, port, tostring(id), guid)
    return sock
end

function adbus.connect_external(host, port, id)
    local sock = socket.connect(host, port)
    sock:send("\0")

    local sendauthline = "AUTH EXTERNAL " .. hex_encode(tostring(id)) .. "\r\n"
    sock:send(sendauthline)

    local okline = sock:receive()
    local guid = okline:match("OK (%x+)")
    assert(guid)

    sock:send("BEGIN\r\n")
    printf("Connected to tcp:host=%s,port=%d with id %s and bus guid %s", host, port, tostring(id), guid)
    return sock
end



