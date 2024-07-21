require("lua.asm")
require("lua.compiler")
require("lua.linker")
require("lua.util")

function CheckSystemFamily()
	local sys = {}

	Check("Determining operating system family")
	-- Every Microsoft operating system since PC-DOS 2 sets %COMSPEC%
	local env = os.getenv("COMSPEC")

	if env then
		sys.divider = "\\"
		sys.md = "MD"
		sys.rm = "DEL"

		-- Every NT operating system sets %OS%
		env = os.getenv("OS")
		if env and env == "Windows_NT" then
			sys.family = "Nt"
			sys.rd = "RD /S /Q"
		else
			sys.family = "Dos"
			sys.rd = "DELTREE /Y"
		end
	else -- Assume it's a Unix system
		sys.divider = "/"
		sys.family = "Unix"
		sys.md = "mkdir"
		sys.rd = "rm -R"
		sys.rm = "rm"
	end

	Pass(sys.family)
	return sys
end

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
	if not cc then
		Check("Guessing assembler")
		if makefile == "cc" then Fail("None") -- Nothing was specified
		elseif makefile == "wcc" or makefile == "wcc386" then
			Pass("Open Watcom")
			CheckWasmAssembler(as, tmpName)
		else
			Pass("GNU Compiler Collection or compatible")
			CheckGccAssembler(as, tmpName)
		end
	else
		CheckCustomAssembler(as, tmpName)
	end
end

function CheckCompiler(makefile)
	local tmpName = UniqueName()

	if not makefile then makefile = Target.makefile end
	if not CreateCTestFile(tmpName .. ".c") then Error() end

	local cc = CheckEnvVar("CC")
	if not cc then
		Check("Guessing compiler")

		if makefile == "cc" then Fail("None") -- Nothing was specified
		elseif makefile == "wcc" or makefile == "wcc386" then
			Pass("Open Watcom")
			if makefile == "wcc386" then CheckWcc386Compiler(tmpName)
			else CheckWccCompiler(tmpName) end
		else
			Pass("GNU Compiler Collection or compatible")
			if makefile == "clang" then
				cc = "clang"
			else
				cc = "gcc"
			end
			CheckGccCompiler(cc, tmpName)
		end
	else
		return CheckCustomCompiler(cc, tmpName)
	end

	os.remove(tmpName .. ".c")
end

function CheckLinker()
	local ld = CheckEnvVar("LD")
	if ld then Compiler.ld = ld
	else
		Check("Guessing linker")
		if Compiler.type == "watcom" then
			Compiler.ld = "wlink"
			if Target.skip == true then
				Pass("Skipped (assuming '" .. Compiler.ld .. "')")
				return
			end
			Pass(Compiler.ld)
			CheckWlinkLinker()
		elseif Compiler.type == "gcc" then
			Compiler.ld = "ld"
			if Target.skip == true then
				Pass("Skipped (assuming '" .. Compiler.ld .. "')")
				return
			end
			Pass(Compiler.ld)
			CheckLdLinker()
		else Fail("Unknown") end
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

function CheckDirContains(dir, files)
	Check("Checking '" .. dir .. "' contains required files")

	for _, file in ipairs(files) do
		local path = SanitizePath(dir .. "/" .. file)

		if not FileExists(path) then Fail("Missing '" .. file .. "'") end
	end

	Pass("Yes")
	return true
end
