-- vim: ts=4 sw=4 sts=4 et

newoption {
    trigger = "to",
    value   = "path",
    description = "Set the output location for the generated files"
}

newaction {
    trigger = "embed",
    description = "Embed lua code into c code to be built into the library",
    execute = function()
        local builddir = _OPTIONS.to or "build"
        local luac = path.join(builddir, "luac")
        luaembed(luac, {
            ["adbuslua"]             = "adbuslua/init.lua",
            ["adbuslua.connection"]  = "adbuslua/connection.lua",
            ["adbuslua.interface"]   = "adbuslua/interface.lua",
            ["adbuslua.object"]      = "adbuslua/object.lua",
            ["adbuslua.pretty"]      = "adbuslua/pretty.lua",
            ["adbuslua.proxy"]       = "adbuslua/proxy.lua",
            ["luaxml"]               = "luaxml/init.lua",
        })
    end
}

function default_location(path)
  if not _OPTIONS.to then
    location(path)
  end
end

solution "adbus"
    configurations {"debug", "release"}
    location(_OPTIONS.to)
    targetdir(_OPTIONS.to or "build")
    configuration {"windows"}
        defines {"_CRT_SECURE_NO_WARNINGS",
                 "WIN32",
                 "LUA_BUILD_AS_DLL",
                 "ADBUS_BUILD_AS_DLL"}
        includedirs {"deps/msvc"}
    configuration {"debug"}
        defines {"DEBUG"}
        flags {"Symbols"}
    configuration {"release"}
        defines {"NDEBUG"}
        flags {"Optimize"}

dofile "projects.lua"

