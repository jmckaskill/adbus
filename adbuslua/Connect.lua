#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

require "package"

package.cpath = package.cpath .. ";/home/james/src/adbus/lib/lib?.so"

require "math"
require "os"
require "io"
require "sha1"
--require "socket"
require "adbuslua_core"

local function connect(host, port, uid)
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

    local sock = socket.connect(host, port)
    sock:send("\0")

    local sendauthline = "AUTH DBUS_COOKIE_SHA1 " .. hex_encode(tostring(uid)) .. "\r\n"
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
    printf("Connected to tcp:host=%s,port=%d with uid %d and bus guid %s", host, port, uid, guid)
end

math.randomseed(os.time())

--connect("localhost", 12345, 1010)

local adbus = adbuslua_core

local function echo(object, foo)
    print(object.str, foo)
end

local interface = adbus.interface.new("nz.co.foobar.ADBus.Test", { 
  { type = "method",
    name = "Bazify",
    arguments = {{ name = "foo", type = "s", direction = "in"},
                 --{ name = "bar", type = "s", direction = "out"}
                },
    --annotations = { "nz.co.foobar.ADBus.Foobar"] = "Test"},
    callback = echo,
  },
  --[[
  { type = "signal",
    name = "Changed",
    arguments = {{ name = "foo", type = "s", direction = "in"}}
    --annotations = { ["nz.co.foobar.ADBus.Foobar"] = "Test"}
  },
  { type = "property",
    name = "Changed",
    property_type = "s",
    --annotations = { ["nz.co.foobar.ADBus.Foobar"] = "Test"},
    get_callback = adbus.connection.get_foobar,
    set_callback = adbus.connection.set_foobar,
  },
  --]]
})
print(interface:name())

--local table = { str = "some string" }

local connection = adbus.connection.new()
local foo = connection:add_object("/foo")

foo.bind_interface(foo, interface, table)



