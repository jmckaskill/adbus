newoption {
  trigger = "to",
  value   = "path",
  description = "Set the output location for the generated files"
}

function default_location(path)
  if not _OPTIONS.to then
    location(path)
  end
end

solution "adbus"
  configurations {"debug", "release"}
  includedirs { ".", "lua-5.1.4/src" }
  location(_OPTIONS.to)
  targetdir "build"
  configuration {"windows"}
    defines {"_CRT_SECURE_NO_WARNINGS",
             "WIN32",
             "LUA_BUILD_AS_DLL",
             "ADBUS_BUILD_AS_DLL"}
  configuration {"linux"}
    defines {"PLATFORM_UNIX"}
    buildoptions {"-fPIC"}
  configuration {"debug"}
    defines {"DEBUG"}
    flags {"Symbols"}
  configuration {"release"}
    defines {"NDEBUG"}
    flags {"Optimize"}

dofile "projects.lua"

