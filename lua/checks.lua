--[[
	check.lua contains functions for miscellaneous yet common checks.
]]

function CheckEnvVar(var)
	Check("Checking '" .. var .. "' enviroment variable")
	v = os.getenv(var)

	if not v then
		Pass("None")
	else
		Pass("'" .. v .. "'")
	end

	return v
end

function CheckAssembler(makefile)
	local tmpName = UniqueName()
	if not makefile then makefile = Target.makefile end

	local as = CheckEnvVar("AS")
	if not as then
		require("lua.asm")
		Check("Guessing assembler")
		if makefile == "cc" then Fail("None") -- Nothing was specified
		elseif makefile == "watcom" then
			Pass("Open Watcom")
			CheckWasmAssembler(as, tmpName)
		else
			Pass("GNU Compiler Collection or compatible")
			CheckGccAssembler(as, tmpName)
		end
	else
		CheckCustomAssembler(as, tmpName)
	end

	CheckNasmAssembler(tmpName)
end

function CheckCompiler(makefile)
	require("lua.compiler")
	local tmpName = UniqueName()

	if not makefile then makefile = Target.makefile end
	if not CreateCTestFile(tmpName .. ".c") then Error() end

	local cc = CheckEnvVar("CC")
	if not cc then
		Check("Guessing compiler")

		if makefile == "cc" then Fail("None") -- Nothing was specified
		elseif makefile == "watcom" then
			Pass("Open Watcom")
			require("lua.compiler.watcom")
		else
			Pass("GNU Compiler Collection or compatible")
			if makefile == "clang" then
				cc = "clang"
			else
				cc = "gcc"
			end
			require("lua.compiler.gcc")
		end
	else
		require("lua.compiler.custom")
	end

	CheckCompiler(cc, tmpName)

	os.remove(tmpName .. ".c")
end

function CheckDosEmu()
	Check("Checking 'dosemu' is available")

	if not Target.xcom then Pass("Not required") return end

	local filename = UniqueName()
	local file = io.open("emu.cfg", "w")
	if not file then Error() end

	file:write([[$_layout = "us"]]) -- This prevents a halt in dosemu
	file:close()

	RunCommand([[dosemu -f emu.cfg -dumb "echo test" > ]] .. filename .. ".txt")
	file = io.open(filename .. ".txt", "r")
	if not file then Error() end
	local txt = file:read()
	file:close()
	if txt:gsub("%s+$", "") == "test" then
		Pass("Yes")
		System.emu = [[dosemu -f emu.cfg -dumb]]
	else
		Fail("No")
	end
end

function CheckRemoveFileCmd(filename)
	Check("Checking '" .. System.rm .. "' works")

	local path = SanitizePath(filename .. "/2.txt")

	file = io.open(path, "a")

	if not file then Error() end

	file:close()

	local e = System.rm .. " " .. path
	RunCommand(e)

	file = io.open(path)
	if file then
		file:close()
		Fail("No")
	end

	Pass("Yes")
	return exec
end

function CheckRemoveDirCmd(filename)
	Check("Checking '" .. System.rd .."' works")

	-- Ensure there's a file in the directory
	local path = SanitizePath(filename .. "/1.txt")
	local file = io.open(path)

	if not file then Error() end
	file:write("Delete me!")
	file:close()

	-- Delete the folder
	local e = System.rd .. " " .. filename
	RunCommand(e)

	-- The file shouldn't open since its folder has been deleted
	local file = io.open(path)
	if file then
		file:close()
		os.remove(path)
		Fail("No")
	end

	Pass("Yes")
end

function CheckCreateDirCmd(filename)
	Check("Checking '" .. System.md .. "' works")

	local e = System.md .. " " .. filename
	RunCommand(e)

	file = io.open(filename .. "/1.txt", "a")

	if file then
		file:close()
		Pass("Yes")
	else Fail("No") end
end
