#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

require("adbuslua")

local function print_table(table)
    for k,v in pairs(table) do
        print (k,v)
    end
end

local win32 = adbuslua.getlocalid():sub(1,1) == "S"
local sock
if win32 then
  sock = adbuslua.connect_external("localhost", 12434, adbuslua.getlocalid())
else
  sock = adbuslua.connect_dbus_cookie_sha1("localhost", 12345, adbuslua.getlocalid())
end

local function echo(object, message)
    print_table(object)
    print_table(message)
    adbuslua.send_reply(message, {message[1]})
end

local interface = adbuslua.interface.new("nz.co.foobar.adbuslua.Test", { 
  { type = "method",
    name = "Echo",
    arguments = {{ name = "in", type = "s", direction = "in"},
                 { name = "out", type = "s", direction = "out"}
                },
    --annotations = { "nz.co.foobar.adbuslua.Foobar"] = "Test"},
    callback = echo,
  },
  --[[
  { type = "signal",
    name = "Changed",
    arguments = {{ name = "foo", type = "s", direction = "in"}}
    --annotations = { ["nz.co.foobar.adbuslua.Foobar"] = "Test"}
  },
  { type = "property",
    name = "Changed",
    property_type = "s",
    --annotations = { ["nz.co.foobar.adbuslua.Foobar"] = "Test"},
    get_callback = adbuslua.connection.get_foobar,
    set_callback = adbuslua.connection.set_foobar,
  },
  --]]
})
print(interface:name())

local table = { str = "some string" }

connection = adbuslua.connection.new(sock)
foo = connection:add_object("/foo")
foo:bind_interface(interface, table)

connection:connect_to_bus()

bus = connection:new_proxy{
    path = "/",
    service = "org.freedesktop.DBus",
    interface = "org.freedesktop.DBus"
}

local names = bus:ListNames()
for k,v in pairs(names) do print(k,v) end

--connection:process_messages()


--[[
message = {
  {"argument 1", 1},
  {arg2 = 3, b = "c"},
  type = "method_call",
  no_reply_expected = true,
  no_auto_start = true,
  serial = 3,
  interface = "nz.co.foobar.adbuslua.Test",
  path = "/nz/co/foobar/adbuslua/Test",
  member = "SomeMethod",
  error_name = "some_error",
  reply_serial = 34,
  destination = "org.freedesktop.DBus",
  sender = ":1.45",
  signature = {"av", "a{sv}"},
}


local interface = adbuslua.interface.new("nz.co.foobar.adbuslua.Test", { 
  { type = "method",
    name = "Bazify",
    arguments = {{ name = "foo", type = "s", direction = "in"},
                 { name = "bar", type = "s", direction = "out"}
                },
    annotations = { nz.co.foobar.adbuslua.Foobar = "Test"},
    callback = adbuslua.connection.foobar,
  },
  { type = "signal",
    name = "Changed",
    arguments = {{ name = "foo", type = "s", direction = "in"}}
    annotations = { nz.co.foobar.adbuslua.Foobar = "Test"}
  },
  { type = "property",
    name = "Changed",
    property_type = "s",
    annotations = { nz.co.foobar.adbuslua.Foobar = "Test"},
    get_callback = adbuslua.connection.get_foobar,
    set_callback = adbuslua.connection.set_foobar,
  },
})

local connection = adbuslua.connection.new()

connection:send_message(....)

local object = connection.add_object("/some/foobar")
object:bind_interface(interface, foobar)
object:emit(interface, "Changed", "new string return value")

local match = connection:add_bus_match{
  type = "signal|error|method_call|method_return",
  serial = 23,
  sender = "org.freedesktop.DBus",
  destination = "org.freedesktop.DBus",
  interface = "org.freedesktop.Hal.Manager",
  path = "/org/freedesktop/Hal/Manager",
  callback = {get_foobar, some_object},
}

connection:remove_bus_match(match)

connection:add_match{
  type = "signal|error|method_call|method_return",
  serial = 23,
  sender = "org.freedesktop.DBus",
  destination = "org.freedesktop.DBus",
  interface = "org.freedesktop.Hal.Manager",
  path = "/org/freedesktop/Hal/Manager",
  unpack_message = true,
  remove_on_first_match = true,
  callback = {get_foobar, some_object},
}



-- Proxy prototype

local proxy = connection:new_proxy{interface = "some.foo.bar", path = "/foo/bar", service = "org.freedesktop.DBus"}

local ret1, ret2 = proxy:Echo("test")
--]]


