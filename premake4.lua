-- vim: ts=4 sw=4 sts=4 et

newoption {
    trigger     = 'to',
    value       = 'path',
    description = 'Set the output location for the generated files'
}

solution 'adbus'
    configurations {'debug', 'release'}
    location(_OPTIONS.to)
    targetdir(_OPTIONS.to or 'build')
    configuration 'debug'
        defines {'DEBUG'}
        flags {'Symbols'}
    configuration 'release'
        defines {'NDEBUG'}
        flags {'Optimize'}
    configuration 'windows'
        defines {'UNICODE'}
        

include 'deps'
dofile 'projects.lua'
dofile 'test-projects.lua'
dofile 'example-projects.lua'

