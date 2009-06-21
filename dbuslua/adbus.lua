-- vim: ts=4 sw=4 sts=4 et

c = dbus.connection.new()
o = c:add_object("/")
i = c:add_interface("nz.co.foobar.DBusLua.Test")

i:add_method{{a.member_function, a},
             "MethodName",
             {"ArgumentName", "s", "in"},
             {"ReturnArg1", "d", "out"},
             com.ctct.foobar.DBusLua.SomeRandomAnnotation = "TheAnnotation"
            }

i:add_signal("SignalName",
             {"ArgumentName", "s", "in"})


dbus = {}
dbus.connection = {}

function dbus.connection.new()
    local o = {}
    setmetatable(o, dbus.connection)
    o._parser = dbusclient.parser()
    o._parser:set_parse_callback(o._parse_callback, o)
    o._marshaller = dbusclient.marshaller()

    -- All callbacks use the same format: a table with index 1 = function and
    -- the rest are unpacked as the first arguments. Thus to bind
    -- a:some_method it would contain {a.some_method, a}

    -- Return registrations
    -- Key: uint32 serial
    -- Value table fields:
    --      callback -- array with 1 = function, rest = first arguments
    --      error_callback -- ..
    o._returns = {}

    -- Signal registrations
    -- Index: arbitrary number
    -- Valid fields:
    -- path, interface, sender, member -- dbus fields to filter on
    -- callback -- array with 1 = function, rest = first arguments
    o._signals = {}

    -- Object registrations
    -- Key: path
    -- Values: object table
    o._objects = {}
    return o
end

function dbus.connection:_parse_callback(message)
    local message_type = message:message_type()
    if message_type == "signal" then
        self:_dispatch_signal(message)
    elseif message_type == "method_call" then
        self:_dispatch_method_call(message)
    elseif message_type == "method_return" then
        self:_dispatch_method_return(message)
    elseif message_type == "error" then
        self:_dispatch_error(message)
    end
end

function dbus.connection:_dispatch_signal(message)
    local path = message:path()
    local interface = message:interface()
    local sender = message:sender()
    local member = message:member()

    for reg in self._signals do
        if  (reg.path == nil or reg.path == path)
        and (reg.interface == nil or reg.interface == interface)
        and (reg.sender == nil or reg.sender == sender)
        and (reg.member == nil or reg.member == member)
        then
            local func = reg.callback[1]
            -- We can not return values/errors in response to a signal
            func(unpack(reg.callback, 2))
        end
    end
end

function dbus.connection:_send_error(message, error_name, error_message)
    self._marshaller:clear()
    self._marshaller:set_send_callback(self._send_callback, self)
    self._marshaller:set_serial(self.next_serial)
    self._marshaller:set_message_type("error")
    self._marshaller:set_error_name(error_name)

    local reply_serial = message:serial()
    self._marshaller:set_reply_serial(reply_serial)

    if error_message ~= nil then
        self._marshaller:begin_argument("s")
        self._marshaller:append_string(error_name)
        self._marshaller:end_argument()
    end

    self._marshaller:send()

    self.next_serial = self.next_serial + 1
end

function dbus.connection:_get_return_marshaller(message)
    self._marshaller:clear()
    self._marshaller:set_send_callback(self._send_callback, self)
    self._marshaller:set_serial(self.next_serial)
    self._marshaller:set_message_type("method_return")

    local reply_serial = message:serial()
    self._marshaller:set_reply_serial(reply_serial)

    self.next_serial = self.next_serial + 1

    return self._marshaller
end


function dbus.connection:_dispatch_method_call(message)
    local path = message:path()
    local object = o._objects[path]
    if object == nil then
        self:_send_error("nz.co.foobar.DBusLua.Error.NoObject", "No object found")
    else
        local ret = { object:_dispatch_method_call(message) }
        if ret[1] == nil and ret[2] ~= nil then
            self:_send_error(message, ret[2], ret[3])
        else
        end
    end
end

dbus.object={}

function dbus.object.new(connection, path)
    local o = {}
    setmetatable(o, dbus.object)

    --Key: interface names
    --Values: interface table
    o._interfaces = {}

    return o
end

function dbus.object:introspect()
end



dbus.interface={}

function dbus.interface.new(object, name)
    local o = {}
    setmetatable(o, dbus.interface)

    o.name = name

    --Key: member names
    --Valid table fields:
    --    type -- member type ("signal", "method", or "property")
    --    callback -- callback for "method"
    --    setter -- callback for "property"
    --    getter -- callback for "property"
    o._methods = {}
    o._signals = {}
    o._properties = {}

    return o
end


function dbus.interface:introspect()
    local function introspect_annotations(v)
        for ak, av in pairs(v.annotations) do
            ret = ret
                + "   <annotation name=\""
                + ak
                + "\" value=\""
                + av
                + "\"/>\n"
        end
    end

    local function introspect_arguments(v)
        for ak, av in pairs(v.in_arguments) do
            ret = ret
                + "   <arg name=\""
                + ak
                + "\" value=\""
                + av
                + "\" direction=\"in\"/>\n"
        end

        for ak, av in pairs(v.out_arguments) do
            ret = ret
                + "   <arg name=\""
                + ak
                + "\" value=\""
                + av
                + "\" direction=\"out\"/>\n"
        end
    end

    local ret = " <interface name=\"" 
              + self.name
              + "\">\n"

    for k,v in pairs(self._methods) do
        ret = ret 
            + "  <method name=\""
            + k
            + "\">\n"
            + introspect_annotations(v)
            + introspect_arguments(v)
            + "  </method>\n"
    end

    for k,v in pairs(self._signals) do
        ret = ret 
            + "  <signal name=\""
            + k
            + "\">\n"
            + introspect_annotations(v)
            + introspect_arguments(v)
            + "  </signal>\n"
    end

    for k,v in pairs(self._signals) do
        ret = ret 
            + "  <property name=\""
            + k
            + "\" type=\""
            + v.type
            + "\">\n"
            + introspect_annotations(v)
            + "  </property>\n"
    end

    ret = ret + " </interface>\n"

    return ret
end

local function sep_args_annotations(data)
    local r = {}
    for k,v in pairs(data) do
        if type(k) == "number" then
            if v[3] == "in" then
                local t = {}
                t.name = v[1]
                t.type = v[2]
                r.in_arguments[k] = t
            elseif v[3] == "out" then
                local t = {}
                t.name = v[1]
                t.type = v[2]
                r.out_arguments[k] = t
            else
                assert(false)
            end
        elseif type(k) == "string" then
            r.annotations[k] = v
        else
            assert(false)
        end
    end
    return r
end

local function next_lua_argument(message, typestring)
    message_type = message:get_signature()
end


--[[
i:add_method{{a.member_function, a},
             "MethodName",
             {"ArgumentName", "s", "in"},
             {"ReturnArg1", "d", "out"},
             com.ctct.foobar.DBusLua.SomeRandomAnnotation = "TheAnnotation"
            }
--]]

function dbus.interface:add_method(data)
    local callback = table.remove(data, 1)
    local name = table.remove(data, 1)
    local reg  = sep_arg_annotations(data)
    reg.callback = callback
    self._methods[name] = reg
end

function dbus.interface:_has_method(name)
    return self._methods[name] ~= nil
end

function dbus.interface:_dispatch_method(name, message)
    local m = self._methods[name]

end

--[[
i:add_signal("SignalName",
             {"ArgumentName", "s", "in"})
--]]

function dbus.interface:add_signal(data)
    local name = table.remove(data, 1)
    local reg = sep_arg_annotations(data)
    self._signals[name] = reg
end

function dbus.interface:add_property(data)
end
