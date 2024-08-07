--[[
	This script scans a file for a string and replaces every instance of
	a posix shell like variable, for example: $(VAR)

	Usage: lua findrepl.lua [file] [VAR] [replace] (> [output file])
]]

-- Need two arguments to run
if #arg < 3 then os.exit(1) end

-- Check file can be open and read
local file = io.open(arg[1], "r")
if not file then os.exit(1) end

-- Read the file line by line to fully construct the value
local var = ""
local match = [[%$%(]] .. arg[2] .. [[%)]]
for line in file:lines() do
	var = var .. line:gsub(match, arg[3])
end
file:close()
print(var)

os.exit(0)
