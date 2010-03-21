#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et

require 'adbus'
require 'os'

-- try and create a random service
local service = os.tmpname()
service = service:gsub('/', '.')
service = service:gsub('\\', '.')
service = service:gsub('^\.', '')

local finish

local c = adbus.connect()
local bus = c:proxy('org.freedesktop.DBus', '/org/freedesktop/DBus')
bus:_connect{
    member      = 'NameOwnerChanged',
    arguments   = {service},
    callback    = function(serv, old, new)
        finish = (new ~= '')
    end,
}

os.execute('lua daemon.lua ' .. service .. '&')

while not finish do
    c:process()
end

c:close()

print('Launched', service)

return service
