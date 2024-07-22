--[[
	gcc.lua contains the functions needed to test
	GNU Compiler Collection compatible C compilers
	including Clang and DJGPP and of cource GCC.
]]

function CheckCompiler(cc, tmpName)
	local gcc = cc or "gcc"

	Compiler.type = "gcc"
	Compiler.output = "-o "
	Compiler.ld = "ld"

	Check("Checking '" .. gcc .. "' is available")

	if Target.skip then
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		Compiler.cc = gcc
		Compiler.cl = gcc
		Compiler.pp = Compiler.cc .. " -E -P"
		return
	end

	RunCommand (
		gcc ..
		" " ..
		tmpName .. ".c"
	)

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = gcc
	Compiler.cl = gcc
	Compiler.pp = Compiler.cc .. " -E -P"

	Check("Checking if '" .. gcc .. "' can target i386")
	RunCommand (
		gcc ..
		" -m32 -march=i386 " ..
		tmpName .. ".c"
	)

	exist = CheckAndRemoveCommonArtifacts(tmpName)
	local m32 = false
	if exist > 0 then
		Pass("Yes")
		m32 = true
	else Pass("No") end

	Check("Checking if '" .. gcc .. "' can target x86-64")
	RunCommand (
		gcc ..
		" -m64 -march=x86-64 " ..
		tmpName .. ".c"
	)

	exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then
		Pass("Yes")
		Compiler.cc64 = gcc
		Compiler.cl64 = gcc
		Compiler.pp64 = Compiler.cc64 .. " -E -P"
	elseif not m32 then Fail("No") -- Need at least one target
	else Pass("No") end -- Continue (32-bit only)

	Check("Checking if '" .. gcc .. "' understands '-fdiagnostics-color=never'")
	RunCommand (
		gcc ..
		" -fdiagnostics-color=never " ..
		tmpName .. ".c"
	)

	exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then
		Pass("Yes")
		Compiler.colorOption = true
	else Pass("No") end
end

function CheckLinker()
	Check("Checking ld works")
	local tmpName = UniqueName()

	if not CreateCTestFile(tmpName .. ".c") then Error() end

	RunCommand (
		Compiler.cc .. " -c " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)

	local file = io.open(tmpName .. ".o")
	if not file then Error() end
	file:close()

	RunCommand(Compiler.ld .. " -e 0 " .. tmpName .. ".o")
	os.remove(tmpName .. ".c")

	local exists = CheckAndReturnCommonExecutable(tmpName)
	CheckAndRemoveCommonArtifacts(tmpName)

	if exists then Pass("Yes") else Fail("No") end
end
