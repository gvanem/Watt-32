--[[
	This script scans a makefile for a variable name and prints it out
	in it's entirety. This is particuraly useful for makefiles
	under DOS where COMMAND.COMs 128 character limit is frankly not
	enough for all the compiler flags to be expressed.

	The script can concatinate += operators of the value together but
	can not resolve symbols on shell executions of variables

	Usage: lua resfile.lua [makefile] [variable] (> [output file])
]]

-- Need two arguments to run
if #arg ~= 2 then os.exit(1) end

-- Check file can be open and read
local file = io.open(arg[1], "r")
if not file then os.exit(1) end

-- Read the file line by line to fully construct the value
local var = ""
for line in file:lines() do
    local val = line:match("^" .. arg[2] .. "%s*=%s*(.*)")
    if val then var = val else
        val = line:match("^" .. arg[2] .. "%s*%+=%s*(.*)")
        if val then var = var .. ' ' .. val end
    end
end

-- Close the file, output the result
makefile:close()
print(var)
os.exit(0)
