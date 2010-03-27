-------------------------------------------------------------------------------
-- vim: ts=4 sw=4 sts=4 et tw=78
--
-- Copyright (c) 2009 James R. McKaskill
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the "Software"),
-- to deal in the Software without restriction, including without limitation
-- the rights to use, copy, modify, merge, publish, distribute, sublicense,
-- and/or sell copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.
--
-------------------------------------------------------------------------------

smodule 'adbus'
require 'xml'
require 'table'
require 'string'

-------------------------------------------------------------------------------
-- Proxy API
-------------------------------------------------------------------------------

_M.service_proxy = {}

local mt = {}

--- Gets the connection object associated with the proxy
function _M.service_proxy.connection(self)
    return rawget(self, "_p").connection
end

--- Gets the name of the remote
function _M.service_proxy.service(self)
    return rawget(self, "_p").service
end

-- Nodes

function mt:__tostring()
    return _M.service_proxy.help(self)
end

function _M.service_proxy.new(connection, service)
    local self = {}
    setmetatable(self, mt)

    local p = {}
    p.service           = service
    p.connection        = connection
    rawset(self, "_p", p)

    rawset(self, "_help", _M.service_proxy.help)
    rawset(self, "_connection", _M.service_proxy.connection)
    rawset(self, "_service", _M.service_proxy.service)

    return self
end

-- Introspection XML processing
local call_introspection

local function process_introspection(self, xml_str, path, paths)
    local c = _M.service_proxy.connection(self)
    local s = _M.service_proxy.service(self)
    local x = xml.eval(xml_str)

    table.insert(paths, path)
    paths.left = paths.left - 1

    for _,node in ipairs(x) do
        if node:tag() == "node" then
            call_introspection(self, path, node.name, paths)
        end
    end

    if paths.left == 0 then
        paths.yield = true
    end
end

call_introspection = function(self, path, node, paths)
    local c = _M.service_proxy.connection(self)
    local s = _M.service_proxy.service(self)
    local node_path = path .. (path == "/" and "" or "/") .. node

    c:async_call{
        destination = s,
        path        = node_path,
        interface   = "org.freedesktop.DBus.Introspectable",
        member      = "Introspect",
        callback    = function(xml) 
            process_introspection(self, xml, node_path, paths)
        end,
    }
    paths.left = paths.left + 1
end

function _M.service_proxy.help(self)
    local c = _M.service_proxy.connection(self)
    local s = _M.service_proxy.service(self)

    local paths = {left = 0}
    call_introspection(self, '/', '', paths)

    c:process(paths)
    table.sort(paths)
    return table.concat(paths, "\n")
end


