#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

require "adbus"


local win32 = adbus.getlocalid():sub(1,1) == "S"
local sock
if win32 then
  sock = adbus.connect_external("localhost", 12434, adbus.getlocalid())
else
  sock = adbus.connect_dbus_cookie_sha1("localhost", 12345, adbus.getlocalid())
end

local function echo(object, message)
    print_table(object)
    print_table(message)
    adbus.send_reply(message, {message[1]})
end

local interface = adbus.interface.new("nz.co.foobar.ADBus.Test", { 
  { type = "method",
    name = "Echo",
    arguments = {{ name = "in", type = "s", direction = "in"},
                 { name = "out", type = "s", direction = "out"}
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

local table = { str = "some string" }

local connection = adbus.connection.new(sock)
local foo = connection:add_object("/foo")
foo:bind_interface(interface, table)

connection:connect_to_bus()

connection:process_messages()


--[[
message = {
  {"argument 1", 1},
  {arg2 = 3, b = "c"},
  type = "method_call",
  no_reply_expected = true,
  no_auto_start = true,
  serial = 3,
  interface = "nz.co.foobar.ADBus.Test",
  path = "/nz/co/foobar/ADBus/Test",
  member = "SomeMethod",
  error_name = "some_error",
  reply_serial = 34,
  destination = "org.freedesktop.DBus",
  sender = ":1.45",
  signature = {"av", "a{sv}"},
}


local interface = adbus.interface.new("nz.co.foobar.ADBus.Test", { 
  { type = "method",
    name = "Bazify",
    arguments = {{ name = "foo", type = "s", direction = "in"},
                 { name = "bar", type = "s", direction = "out"}
                },
    annotations = { nz.co.foobar.ADBus.Foobar = "Test"},
    callback = adbus.connection.foobar,
  },
  { type = "signal",
    name = "Changed",
    arguments = {{ name = "foo", type = "s", direction = "in"}}
    annotations = { nz.co.foobar.ADBus.Foobar = "Test"}
  },
  { type = "property",
    name = "Changed",
    property_type = "s",
    annotations = { nz.co.foobar.ADBus.Foobar = "Test"},
    get_callback = adbus.connection.get_foobar,
    set_callback = adbus.connection.set_foobar,
  },
})

local connection = adbus.connection.new()

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


