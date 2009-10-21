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
  location(_OPTIONS.to)
  targetdir(_OPTIONS.to)
  configuration {"debug"}
    defines {"DEBUG"}
    flags {"Symbols"}
  configuration {"release"}
    defines {"NDEBUG"}
    flags {"Optimize"}

dofile "projects.lua"

