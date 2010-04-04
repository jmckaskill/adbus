require 'adbus'
c = _CONNECTION
p = adbus.proxy.new(c, 'nz.co.foobar.adbus.PingServer', '/')

function call(str)
    on_send()
    assert(p:Ping(str) == 'str')
    on_reply()
end

function reply(str, err)
    assert(str == 'str', err)
    call(str)
    on_reply()
end

function async_call(str)
    on_send()
    c:async_call{
        callback    = reply,
        destination = 'nz.co.foobar.adbus.PingServer',
        path        = '/',
        member      = 'Ping',
        signature   = 's',
        str
    }
end

