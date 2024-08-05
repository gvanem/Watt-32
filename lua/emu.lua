--[[
	emu.lua contains the functions needed for the script to resolve
	emulators when cross compiling.
]]

function GetWorkingDirectory()
	local name = UniqueName()
	local cmd = System.family == "Unix" and [[pwd > ]] or [[CD > ]]
	RunCommand(cmd .. name .. ".txt")
	cmd = nil

	local file = io.open(name .. ".txt", "r")
	if not file then Error() end

	local txt = file:read()
	print(txt)
	file:close()
	file = nil
	os.remove(name .. ".txt")

	if txt then return txt else Error() end
end

function CheckDosEmu()
	Check("Checking 'dosemu' is available")

	if not Target.xcom then Pass("Not required") return end

	local filename = UniqueName()
	local file = io.open("dosemu.cfg", "w")
	if not file then Error() end

	-- Setting layout prevents a potenical halt in dosemu
	-- full cpu emu required for DPMI features to function
	file:write([[
$_layout = "us"
$_cpu_emu = "full"
]])
	file:close()

	RunCommand([[dosemu -f dosemu.cfg -dumb "echo test" > ]] .. filename .. ".txt")
	file = io.open(filename .. ".txt", "r")
	if not file then Error() end
	local txt = file:read()
	file:close()
	if txt:gsub("%s+$", "") == "test" then
		Pass("Yes")
		System.emu = [[dosemu -f dosemu.cfg -dumb]]
	else
		Fail("No")
	end
end

function CreateBatchScript(exec, filename)
	if not filename then filename = UniqueName() end
	local file = io.open(filename .. ".bat", "w")
	if not file then Error() end

	file:write([["lredir C: linux\fs]] .. GetWorkingDirectory() .. '\n')

	if type(exec) == "string" then file:write(exec .. '\n')
	elseif type(input) == "table" then
		for _, e in ipairs(exec) do
			if type(e) == "string" then file:write(e .. '\n') end
		end
	end
	file:close()

	return filename .. ".bat"
end
