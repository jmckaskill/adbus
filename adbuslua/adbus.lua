-- vim: ts=4 sw=4 sts=4 et

c = adbus.connection.new()
o = c:add_object("/")
i = c:add_interface("nz.co.foobar.ADBusLua.Test")

i:add_method{{a.member_function, a},
             "MethodName",
             {"ArgumentName", "s", "in"},
             {"ReturnArg1", "d", "out"},
             nz.co.foobar.ADBusLua.SomeRandomAnnotation = "TheAnnotation"
            }

i:add_signal("SignalName",
             {"ArgumentName", "s", "in"})


adbus = {}
adbus.connection = {}

function adbus.connection.new()
    local o = {}
    setmetatable(o, adbus.connection)
    o._parser = adbusclient.parser()
    o._parser:set_parse_callback(o._parse_callback, o)
    o._marshaller = adbusclient.marshaller()

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

function adbus.connection:_parse_callback(message)
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

function adbus.connection:_dispatch_signal(message)
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

function adbus.connection:_send_error(message, error_name, error_message)
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

function adbus.connection:_get_return_marshaller(message)
    self._marshaller:clear()
    self._marshaller:set_send_callback(self._send_callback, self)
    self._marshaller:set_serial(self.next_serial)
    self._marshaller:set_message_type("method_return")

    local reply_serial = message:serial()
    self._marshaller:set_reply_serial(reply_serial)

    self.next_serial = self.next_serial + 1

    return self._marshaller
end


function adbus.connection:_dispatch_method_call(message)
    local path = message:path()
    local object = o._objects[path]
    if object == nil then
        self:_send_error("nz.co.foobar.ADBusLua.Error.NoObject", "No object found")
    else
        local ret = { object:_dispatch_method_call(message) }
        if ret[1] == nil and ret[2] ~= nil then
            self:_send_error(message, ret[2], ret[3])
        else
        end
    end
end

adbus.object={}

function adbus.object.new(connection, path)
    local o = {}
    setmetatable(o, adbus.object)

    --Key: interface names
    --Values: interface table
    o._interfaces = {}

    return o
end

function adbus.object:introspect()
end



adbus.interface={}

function adbus.interface.new(object, name)
    local o = {}
    setmetatable(o, adbus.interface)

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


local interface_begin = " <interface name=\"%s\">\n"
local interface_end   = " </interface>\n"
local method_begin    = "  <method name=\"%s\">\n"
local method_end      = "  </method>\n"
local signal_begin    = "  <signal name=\"%s\">\n"
local signal_end      = "  </signal>\n"
local property_begin  = "  <property name=\"%s\" type=\"%s\">\n"
local property_end    = "  </property>\n"
local annotation_xml  = "   <annotation name=\"%s\" value=\"%s\"/>\n"
local arg_xml         = "   <arg name=\"%s\" value=\"%s\" direction=\"%s\"/>\n"

local function introspect_annotations(v)
    local ret = ""
    for ak, av in pairs(v.annotations) do
        ret = ret .. string.format(annotation_xml, ak, av)
    end
    return ret
end

local function introspect_arguments(v)
    local ret = ""
    for ak, av in pairs(v.in_arguments) do
        ret = ret .. string.format(arg_xml, ak, av, "in")
    end
    for ak, av in pairs(v.out_arguments) do
        ret = ret .. string.format(arg_xml, ak, av, "out")
    end
    return ret
end

function adbus.interface:introspect()
    local ret = ""
    local function append(...)
        ret = ret .. string.format(...)
    end

    append(interface_begin, self.name)

    for k,v in pairs(self._methods) do
        append(method_begin, k)
        append(introspect_annotations(v))
        append(introspect_arguments(v))
        append(method_end)
    end

    for k,v in pairs(self._signals) do
        append(signal_begin, k)
        append(introspect_annotations(v))
        append(introspect_arguments(v))
        append(signal_end)
    end

    for k,v in pairs(self._propertiess) do
        append(property_begin, k, v.type)
        append(introspect_annotations(v))
        append(property_end)
    end

    append(interface_end)

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

local function get_array(message, scope)
    local ret = {}
    while not message:is_scope_finished(scope) do
        local field, err = next_lua_field(message)
        if field == nil then return nil, err end
        if field.type == "dict_entry" then
            ret[field.key] = field.value
        else
            ret:insert(field.data)
        end
    end
    local a, err = message:take_array_end()
    if d == nil then return nil, err end

    return {type = "array", data = ret}
end

local function get_struct(message, scope)
    local ret = {}
    while not message:is_scope_finished(scope) do
        local field, err = next_lua_field(message)
        if field == nil then return nil, err end
        ret:insert(field.data)
    end
    local a, err = message:take_struct_end()
    if d == nil then return nil, err end

    return {type = "array", data = ret}
end

local function get_dict_entry(message, scope)
    local key, value, d, err
    key, err = next_lua_field(message)
    if key == nil then return nil, err end

    value, err = next_lua_field(message)
    if value == nil then return nil, err end

    d, err = message:take_dict_entry_end()
    if d == nil then return nil, err end

    return {type = "dict_entry", key = key, value = value}    
end

local function get_variant(message, scope)
    local field, err, v
    field, err = next_lua_field(message)
    if field == nil then return nil, err end

    v,err = message:take_variant_end()
    if v == nil then return nil, err end

    return {type = "variant", data = field}
end

local function next_lua_field(message)
    local field, err = message:next_field()
    if field == nil then return nil, err end

    if field.type == "boolean" then
        return field
    elseif field.type == "string" then
        return field
    elseif field.type == "number" then
        return field
    elseif field.type == "array_begin" then
        return get_array(message, field.scope)
    elseif field.type == "struct_begin" then
        return get_struct(message, field.scope)
    elseif field.type == "variant_begin" then
        return get_variant(message, field.scope)
    elseif field.type == "dict_entry_begin" then
        return get_dict_entry(message, field.scope)
    end
end

local function next_lua_argument(message, typestring)
    local argument_type = message:next_argument_signature()
    if argument_type ~= typestring then
        return nil, dbus._errors.InvalidArgument
    end

    local argbegin, argend, field, err
    argbegin, err = message:take_argument_begin()
    if argbegin == nil then return nil, err end

    field, err = next_lua_field(message)
    if field == nil then return nil, err end

    argend, err = message:take_argument_end()
    if argend == nil then return nil, err end

    return field
end


--[[
i:add_method{{a.member_function, a},
             "MethodName",
             {"ArgumentName", "s", "in"},
             {"ReturnArg1", "d", "out"},
             nz.co.foobar.ADBusLua.SomeRandomAnnotation = "TheAnnotation"
            }
--]]

function adbus.interface:add_method(data)
    local callback = table.remove(data, 1)
    local name = table.remove(data, 1)
    local reg  = sep_arg_annotations(data)
    reg.callback = callback
    self._methods[name] = reg
end

function adbus.interface:_has_method(name)
    return self._methods[name] ~= nil
end

function adbus.interface:_dispatch_method(name, message)
    local m = self._methods[name]

end

--[[
i:add_signal("SignalName",
             {"ArgumentName", "s", "in"})
--]]

function adbus.interface:add_signal(data)
    local name = table.remove(data, 1)
    local reg = sep_arg_annotations(data)
    self._signals[name] = reg
end

function adbus.interface:add_property(data)
end
