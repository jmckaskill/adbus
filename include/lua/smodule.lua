#!/usr/bin/lua
-- vim: ts=4 sw=4 sts=4 et

require 'package'

local function mod_require(x)
    local namespace = getfenv(2)
    local module = require(x)
    -- Split the module name on . and create tables down to that point
    for path in string.gmatch(x, '([%w_]+)%.') do
        namespace = namespace[path]
        if not namespace then namespace = {} end
    end
    local basename = string.match(x, '([%w_]+)$')
    namespace[basename] = module
    return module
end

package.smodule_base = {
    _G = _G,
    _VERSION = _VERSION,
    assert = assert,
    collectgarbage = collectgarbage,
    dofile = dofile,
    error = error,
    getfenv = getfenv,
    getmetatable = getmetatable,
    ipairs = ipairs,
    load = load,
    loadfile = loadfile,
    loadstring = loadstring,
    -- module = module, -- don't copy over the module func as you should only
    -- run it once
    next = next,
    pairs = pairs,
    pcall = pcall,
    print = print,
    rawequal = rawequal,
    rawget = rawget,
    rawset = rawset,
    require = mod_require, --note we don't use the normal require function
    select = select,
    setfenv = setfenv,
    setmetatable = setmetatable,
    tonumber = tonumber,
    tostring = tostring,
    type = type,
    unpack = unpack,
    xpcall = xpcall,
}

if not package.smodules then package.smodules = {} end

local builtin_module = module
local smodules = package.smodules
local base = package.smodule_base
local loaded = package.loaded
local getfenv = getfenv
local setfenv = setfenv
local pairs = pairs
local print = print


function smodule(x)
    builtin_module(x)
    local mod = getfenv()
    local modpriv = smodules[x]
    if not modpriv then
        modpriv = {_NAME = x, _M = mod}
        for k,v in pairs(base) do
            modpriv[k] = v
        end
        smodules[x] = modpriv
    end
    setfenv(2, modpriv)
    return mod
end

