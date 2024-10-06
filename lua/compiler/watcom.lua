--[[
	watcom.lua contains the functions needed to test
	Open Watcoms 1.9 or later C compilers (wcc and wcc386)
]]

local function CheckWccCompiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ar = "wlib"
	Compiler.ld = "wlink"

	Check("Checking wcc is available")

	if Target.skip then
		Compiler.cc16 = "wcc"
		Compiler.cl16 = "wcl"
		Compiler.m16 = true
		Compiler.pp16 = Compiler.cc16 .. " -P"
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		return
	end

	RunCommand (
		"wcc -q " ..
		tmpName .. ".c"
	)

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc16 = "wcc"
	Compiler.cl16 = "wcl -q"
	Compiler.m16 = true
	Compiler.pp16 = Compiler.cc16 .. " -P"
end

local function CheckWcc386Compiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ar = "wlib"
	Compiler.ld = "wlink"

	Check("Checking wcc386 is available")

	if Target.skip then
		Compiler.cc = "wcc386"
		Compiler.cl = "wcl386"
		Compiler.pp = Compiler.cc .. " -P"
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		return
	end

	RunCommand (
		"wcc386 -q " ..
		tmpName .. ".c"
	)

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = "wcc386"
	Compiler.cl = "wcl386 -q"
	Compiler.pp = Compiler.cc .. " -P"
end

function CheckArchiver()
	Check("Checking '" .. Compiler.ar .. "' works")
	local tmpName = UniqueName()

	if not CreateCTestFile(tmpName .. ".c") then Error() end

	if Compiler.cc16 then
	RunCommand (
		Compiler.cc16 .. " " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)
	else
		RunCommand (
		Compiler.cc .. " " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)
	end

	local file = io.open(tmpName .. ".o")
	if not file then Error() end
	file:close()

	RunCommand(Compiler.ar .. " +" .. tmpName .. ".o " .. tmpName .. ".lib")
	os.remove(tmpName .. ".c")

	file = io.open(tmpName .. ".lib")
	CheckAndRemoveCommonArtifacts(tmpName)
	if not file then Fail("No") end
	os.remove(tmpName .. ".lib")
	Pass("Yes")
end

function CheckLinker()
	Check("Checking '" .. Compiler.ld .. "' works")
	local tmpName = UniqueName()

	if not CreateCTestFile(tmpName .. ".c") then Error() end

	if Compiler.cc16 then
	RunCommand (
		Compiler.cc16 .. " " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)
	else
		RunCommand (
		Compiler.cc .. " " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)
	end

	local file = io.open(tmpName .. ".o")
	if not file then Error() end
	file:close()

	RunCommand(Compiler.ld .. " f " .. tmpName .. ".o")
	os.remove(tmpName .. ".c")

	local exists = CheckAndReturnCommonExecutable(tmpName)
	CheckAndRemoveCommonArtifacts(tmpName)

	if exists then Pass("Yes") else Fail("No") end
end

function CheckCompiler(cc, tmpName)
	CheckWccCompiler(tmpName)
	CheckWcc386Compiler(tmpName)

	Check("Is a full watcom toolchain available")
	if (not Compiler.cc and not Compiler.cc16) or not Compiler.ld then
		Fail("No")
	else
		Pass("Yes")
	end
end
