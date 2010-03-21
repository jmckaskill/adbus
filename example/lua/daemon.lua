#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et

require 'adbus'
require 'os'


assert(arg[1])
print(arg[1], "Start")

local c = adbus.connect{
    callback = function(name)
        print(arg[1], "Connected", name)
    end
}
c:request_name(arg[1])

local quit      = false
local obj       = adbus.object.new()
local property  = 'prop'
local prefix    = 'pre'
local bind

local iface = adbus.interface{
    name = 'nz.co.foobar.TestInterface',
    {   'method', 'Method',
        arguments = {{'str', 's'}},
        returns   = {{'echo', 's'}},
        callback  = function(str) 
            print(arg[1], "Method", str) 
            return prefix .. str 
        end,
    },
    {   'method', 'Emit',
        arguments = {{'str', 's'}},
        callback  = function(str)
            print(arg[1], "Emit", str)
            bind:emit('Signal', str) 
        end
    },
    {   'signal', 'Signal',
        arguments = {{'str', 's'}},
    },
    {   'property', 'Property',
        type = 's',
        getter = function() 
            print(arg[1], "Property getter", property)
            return property 
        end,
        setter = function(p) 
            print(arg[1], "Property setter", p)
            property = p 
        end,
    },
    {   'method', 'Quit',
        callback = function()
            obj:close()
            quit = true 
        end,
    },
}

bind = obj:bind(c, '/', iface)

while not quit do
    c:process()
end

obj:close()
c:close()

print(arg[1], "Finish")

