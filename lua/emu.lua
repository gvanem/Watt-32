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

local function CreateDosEmuConfigurationFile(filename)
	local file = io.open(filename .. ".cfg", "w")
	if not file then Error() end

	-- Setting layout prevents a potenical halt in dosemu
	-- full cpu emu required for DPMI features to function
	file:write([[
$_layout = "us"
$_cpu_emu = "full"
]]	)
	file:close()

	return filename .. ".cfg"
end

function CheckDosEmu()
	Check("Checking 'dosemu' is available")

	if not Target.xcom then Pass("Not required") return end

	local filename = UniqueName()

	local cfgFileName = CreateDosEmuConfigurationFile(filename)
	local outFileName = filename .. ".txt"

	RunCommand(
		[[dosemu -f ]] .. cfgFileName ..
		[[ -dumb "echo test" > ]] .. outFileName
	)
	file = io.open(outFileName, "r")
	if not file then Error() end
	local txt = file:read()
	file:close()

	os.remove(cfgFileName)
	os.remove(outFileName)

	if txt:gsub("%s+$", "") == "test" then
		Pass("Yes")
		System.emu = [[dosemu]]
	else
		Fail("No")
	end
end

local function CreateBatchScript(exec, filename)
	if not filename then filename = UniqueName() end
	local file = io.open(filename .. ".bat", "w")
	if not file then Error() end

	if type(exec) == "string" then file:write(exec .. '\n')
	elseif type(exec) == "table" then
		for _, e in ipairs(exec) do
			if type(e) == "string" then file:write(e .. '\n') end
		end
	end
	file:close()

	return filename .. ".bat"
end

function RunCommandEmu(execs, filename)
	-- TODO: Only support dosemu at the moment. Generalize for Dosbox, Wine, Vdos etc...
	if not filename then filename = UniqueName() end

	local batFileName = CreateBatchScript(execs, filename)
	local cfgFileName = CreateDosEmuConfigurationFile(filename)

	RunCommand(System.emu .. [[ -f ]] .. cfgFileName .. [[ -dumb ]] .. batFileName)
	os.remove(cfgFileName)
	os.remove(batFileName)
end
