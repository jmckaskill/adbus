#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et


smodule 'adbus'
require 'adbuslua_core'
require 'xml'
require 'string'

_M.interface = {}

local function errorf(...)
    error(string.format(...), 2)
end

local function add_method(iface, mbr, retsigtable)
    local retsig = ""
    iface:add_method(mbr.name)
    for _,arg in ipairs(mbr) do
        -- Process method item
        if arg:tag() == "arg" then
            if arg.direction == "out" then
                -- Out argument
                retsig = retsig .. arg.type
                iface:return_signature(arg.type)
                if arg.name then iface:return_name(arg.name) end

            elseif arg.direction == "in" or arg.direction == nil then
                -- In argument
                iface:argument_signature(arg.type)
                if arg.name then iface:argname(arg.name) end
            else
                errorf("Invalid argument direction '%s'", arg.direction)
            end
        elseif arg:tag() == "annotation" then
            -- Annotation
            iface:annotate(arg.name, arg.value)
        else
            errorf("Invalid method node '%s'", arg:tag())
        end
    end
    retsigtable[mbr.name] = retsig
end

local function add_signal(iface, mbr, sigtable)
    local sig = ""
    iface:add_signal(mbr.name)
    for _,arg in ipairs(mbr) do
        -- Process signal item
        if arg:tag() == "arg" then
            if arg.direction == "out" or arg.direction == nil then
                -- Out argument
                iface:argument_signature(arg.type)
                sig = sig .. arg.type
                if arg.name then iface:argument_name(arg.name) end
            else
                errorf("Invalid argument direction '%s'", arg.direction)
            end
        elseif arg:tag() == "annotation" then
            -- Annotation
            iface:annotate(arg.name, arg.value)
        else
            errorf("Invalid signal node '%s'", arg:tag())
        end
    end
    sigtable[mbr.name] = sig
end

local function add_property(iface, mbr)
    iface:add_property(mbr.name, mbr.type, mbr.access)
    for _,arg in ipairs(mbr) do
        -- Process property item
        if arg:tag() == "annotation" then
            iface:annotate(arg.name, arg.value)
        else
            errorf("Invalid property node '%s'", arg:tag())
        end
    end
end

function _M.interface.new(introspection)
    local self = {}
    self._signal_signature = {}
    self._method_return_signature = {}

    local x = xml.eval(introspection)

    if not x or x:tag() ~= "interface" then
        error("Invalid interface xml")
    end

    local iface = adbuslua_core.interface.new(x.name)

    for _,node in ipairs(x) do
        local tag = node:tag()
        if tag == "method" then
            add_method(iface, node, self._method_return_signature)
        elseif tag == "signal" then
            add_signal(iface, node, self._signal_signature)
        elseif tag == "property" then
            add_property(iface, node)
        else
            errorf("Invalid interface node '%s'", tag)
        end
    end

    self._interface = iface 
    self.name = x.name
    return self
end



