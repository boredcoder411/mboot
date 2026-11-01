-- Read the file-like CPU information device through Lua I/O.
function readAll(file)
  local f = io.open(file, "rb")
  local content = f:read("*all")
  f:close()
  return content
end

print(readAll("/dev/cpuinfo"))