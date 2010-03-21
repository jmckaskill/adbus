#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et tw=78

require 'adbus'

local service = dofile 'launch-daemon.lua'

print("Connecting to bus")
local c = adbus.connect{
    debug = true,
    callback = function(name)
        print("Connected", name)
    end
}

print("Getting proxy")
local p = c:proxy(service, '/')

local finish

print("Connecting to signal")
id = p:_connect{
    member = "Signal",
    callback = function(str)
        finish = true
        print("Received", str)
    end
}

print("Getting daemon to emit the signal")
p:Emit("test")

print("Running main event loop")
while not finish do
    c:process()
end

print("Getting the daemon to quit")
p:Quit()

