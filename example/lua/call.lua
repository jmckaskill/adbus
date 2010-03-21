#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et

require 'adbus'
require 'pretty'

service = arg[1]
path = arg[2]
member = arg[3]
table.remove(arg, 1)
table.remove(arg, 1)
table.remove(arg, 1)

c = adbus.connect()
bus = c:proxy('org.freedesktop.DBus', '/')

local function list_services()
    names = {}
    for _,n in ipairs(bus:ListNames()) do
        local owner = bus:GetNameOwner(n)
        names[owner] = names[owner] or {}
        table.insert(names[owner], n)
    end

    me = c:unique_name()
    if me then
        table.insert(names[me], "this script")
    end

    names2 = {}
    for k,v in pairs(names) do
        table.insert(names2, {k,v})
    end
    table.sort(names2, function(n1,n2) return n1[1] < n2[1] end)
    for _,n in ipairs(names2) do
        print(n[1])
        for _,s in ipairs(n[2]) do
            if s ~= n[1] then
                print(" - " ..  s)
            end
        end
    end
end

local function is_valid_path(service, path)
    -- call the Introspect method on the path manually and check for an error
    -- message

    local msg = c:call{
        destination     = service,
        path            = path,
        interface       = "org.freedesktop.DBus.Introspectable",
        member          = "Introspect",
        unpack_message  = false,
    }
    return msg.type ~= "error"
end

-- Check the service
if not service then
    return list_services()
elseif not bus:NameHasOwner(service) then
    print("Invalid service")
    return list_services()
end

-- Check the path
if not path then
    return print(c:proxy(service))
elseif not is_valid_path(service, path) then
    print("Invalid path")
    return print(c:proxy(service))
end

-- Check the method
if not member then
    return print(c:proxy(service, path))
else
    -- Run the method, listen to the signal, or set/get the property
    p = c:proxy(service, path)
    t = p:_member_type(member)
    if t == 'method' then
        local f = p[member]
        table.print(f(p, unpack(arg)))
    elseif t == 'signal' then
        p:_connect{
            member = member,
            callback = function(...) print(...) end,
        }
        c:process()
    elseif t == 'property' then
        if arg[1] then
            -- set the property
            p[member] = arg[1]
        else
            -- get the property
            print(p[member])
        end
    else
        print("Invalid member")
        print(p)
    end
end




