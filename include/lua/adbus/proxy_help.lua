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
require 'table'
require 'string'
require 'xml'

_M.proxy = _M.proxy or {}

local function data_string(d, widths, linesep)
    local ret = ""
    for row in ipairs(d[1]) do
        ret = ret .. linesep
        for col in ipairs(d) do
            ret = ret .. string.format(widths[col].format, d[col][row] or "")
        end
        ret = ret .. "\n"
    end
    return ret
end

local function calc_widths(d, widths)
    for col,width in ipairs(widths) do
        local max = width.max or 0
        for _,cell in ipairs(d[col]) do
            if #cell > max then max = #cell end
        end
        widths[col].max = max
        widths[col].format = string.format("%%-%ds  ", max)
    end
end

local function normalise_columns(d)
    local len = 0
    for _,col in ipairs(d) do
        if #col > len then 
            len = #col 
        end
    end
    for _,col in ipairs(d) do
        while #col < len do 
            table.insert(col, "") 
        end
    end
end

function _M.proxy.help(self)
    local ret = ""
    local introspection = self:_introspect()

    local x = xml.eval(introspection)
    local interface = _M.proxy.interface(self)

    local interfaces = {}

    for _,xml_interface in ipairs(x) do
        if xml_interface:tag() == "interface" 
        and (interface == nil or xml_interface.name == interface) 
        then
            local methods = {}
            local props = {}
            local sigs = {}
            for _,member in ipairs(xml_interface) do
                local tag = member:tag()
                if tag == "method" then
                    local args, rets = {}, {}
                    for _,a in ipairs(member) do
                        local s = table.concat({a.type,a.name}, "/")
                        if a.direction == "out" then
                            table.insert(rets, s)
                        else
                            table.insert(args, s)
                        end
                    end
                    table.insert(methods, {name = member.name, rets, args})
                elseif tag == "signal" then
                    local args = {}
                    for _,a in ipairs(member) do
                        table.insert(args, table.concat({a.type,a.name}, "/"))
                    end
                    table.insert(sigs, {name = member.name, args})
                elseif tag == "property" then
                    table.insert(props, member)
                end
            end
            table.insert(interfaces, {
                name = xml_interface.name,
                methods = methods,
                sigs = sigs,
                props = props
            })
        end
    end

    local nodes = rawget(self, "_p").nodes
    local nodelist = {}
    for n in pairs(nodes) do
        table.insert(nodelist, n)
    end
    table.sort(nodelist)

    local s = function(m1,m2) return m1.name < m2.name end


    local tables = {}

    local d = {
        {"Service", "Path", ""}, 
        {_M.proxy.service(self), _M.proxy.path(self)}, 
        {}, 
        {}
    }
    normalise_columns(d)
    table.insert(tables, d)

    if #nodelist > 0 then
        local d = {name = "Children", nodelist, {}, {}, {}}
        normalise_columns(d)
        table.insert(d[1], "")
        table.insert(tables, d)
    end

    for _,i in ipairs(interfaces) do
        local methods = i.methods
        local props = i.props
        local sigs = i.sigs
        table.sort(methods, s)
        table.sort(props, s)
        table.sort(sigs, s)
        local d = {name = i.name, {}, {}, {}, {}}
        if #methods > 0 then
            table.insert(d[1], "Methods:")
            for _,m in ipairs(methods) do
                table.insert(d[2], m.name)
                table.insert(d[3], table.concat(m[1], ", "))
                table.insert(d[4], table.concat(m[2], ", "))
            end
            normalise_columns(d)
        end

        if #sigs > 0 then
            table.insert(d[1], "Signals:")
            for _,s in ipairs(sigs) do
                table.insert(d[2], s.name)
                table.insert(d[3], "")
                table.insert(d[4], table.concat(s[1], ", "))
            end
            normalise_columns(d)
        end

        if #props > 0 then
            table.insert(d[1], "Properties:")
            for _,p in ipairs(props) do
                table.insert(d[2], p.name)
                table.insert(d[3], p.type)
                table.insert(d[4], p.access)
            end
            normalise_columns(d)
        end
        table.insert(d[1], "")
        table.insert(tables, d)
    end

    local widths = {{},{},{},{}}
    for _,table in ipairs(tables) do
        calc_widths(table, widths)
    end

    for i,table in ipairs(tables) do
        if table.name then
            ret = ret .. table.name .. "\n"
        end
        ret = ret .. data_string(table, widths, '\t')
    end

    return ret
end


