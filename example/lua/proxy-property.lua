#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et

require 'adbus'

service = dofile 'launch-daemon.lua'

c = adbus.connect()
p = c:proxy(service, '/')

v = p.Property
print('Got', v)
assert(v == 'prop')

print('Set', 'test')
p.Property = "test"

v = p.Property
print('Got', v)
assert(v == 'test')

p:Quit()


