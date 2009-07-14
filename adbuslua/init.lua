#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

local function include(filename)
    local f,err = loadfile(filename)
    assert(f,err)
    f()
end

include("adbuslua/Connect.lua")
include("adbuslua/Connection.lua")
include("adbuslua/Proxy.lua")

