--[[
	This lua script converts an ASCII characters into to a 
	C hex array for generating cflagsbf.h. 
	It is executed by makefiles
]]

-- Silent exit with error if no readable file can be found
if #arg < 1 then os.exit(1) end
local file = io.open(arg[1], "rb")
if not file then os.exit(1) end

local column = {}
local row = {}
local count = 0

-- Parse the file
while true do
	local byte = file:read(1)

	if not byte then break end  -- EOF

	-- Insert the processed byte into the table
	table.insert(column, string.format("0x%02X", string.byte(byte)))

	-- Newline after every 12th element
	count = count + 1
	if count % 12 == 0 then
		table.insert(row, table.concat(column, ", ") .. ",")
		column = {}
	end
end
file:close()

-- Append last row
table.insert(row, table.concat(column, ", ") .. ",")

-- Write C hex array to stdout
local result = "/* bin2c.lua output begin */\n\t" ..
	table.concat(row, '\n\t') ..
	"\n/* bin2c.lua output end */"
print(result)
os.exit(0)
