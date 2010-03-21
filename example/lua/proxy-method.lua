#!/usr/bin/env lua
-- vim: ts=4 sw=4 sts=4 et

require 'adbus'

service = dofile 'launch-daemon.lua'

c = adbus.connect()
p = c:proxy(service, '/')

r = p:Method("test")
assert(r == "pretest")

p:Quit()


