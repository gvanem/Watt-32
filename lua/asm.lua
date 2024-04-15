require("lua.compiler")
require("lua.util")

function GetExecutableNames(name)
	return {name, "a"}
end

function GetExecutableExtensions()
	return {".com", ".exe", ".out"}
end

function GetCompilerOutputExtensions()
	return {".a", ".com", ".dll", ".exe", ".lib", ".o", ".obj", ".out"}
end

function CheckAndReturnCommonExecutable(baseName)
	local names = GetExecutableNames(baseName)
	local extensions = GetExecutableExtensions()

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				file:close()
				return fileName
			end
		end
	end
end

function CheckAndRemoveCommonArtifacts(baseName)
	local names = GetExecutableNames(baseName)
	local extensions = GetCompilerOutputExtensions()
	local r = 0

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				file:close()
				os.remove(fileName)
				r = r + 1
			end
		end
	end

	return r
end

function CheckCustomAssembler(as, tmpName)
	Check("Checking '" .. as .. "' compiler understands MASM format")

	if Target.skipChecks then
		Pass("Skipped")
		return
	end

	if not CreateMasmTestFile(tmpName .. ".asm") then Error() end
	RunCommand(as .. " " .. tmpName .. ".asm")
	os.remove(tmpName .. ".asm")

	local exists = CheckAndRemoveCommonArtifacts(tmpName)
	if exists > 0 then 
		Pass("Yes") 
		Compiler.as = as
		Compiler.atype = "masm"
	else 
		Pass("No")
		Check("Checking '" .. as .. "' compiler understands GAS format")

		if not CreateGasTestFile(tmpName .. ".s") then Error() end
		RunCommand(as .. " " .. tmpName .. ".s")
		os.remove(tmpName .. ".s")
		exists = CheckAndRemoveCommonArtifacts(tmpName)
		if exists > 0 then 
			Pass("Yes") 
			Compiler.as = as
			Compiler.atype = "gas"
			Pass("Yes")
		end
	end

	local name

	if Compiler.atype == "masm" then
		if not CreateMasmTestFile(tmpName .. ".asm") then Error() else
			name = tmpName .. ".asm"
		end
	else
		if not CreateGasTestFile(tmpName .. ".s") then Error() else
			name = tmpName .. ".s"
		end
	end

	local aflags = CheckEnvVar("AFLAGS")
	if aflags then
		Check("Checking if C compiler works with CFLAGS")
		local r = RunCommand (as .. " " .. aflags .. " " .. name)

		local exists = CheckAndRemoveCommonArtifacts(tmpName)
		if exists > 0 then Pass("Yes")
		else
			os.remove(name)
			Fail("No")
		end

		Compiler.aflags = aflags
	end

	os.remove(name)
end

function CheckGccAssembler(as, tmpName)
	local gcc = as or "as"

	Check("Checking '" .. gcc .. "' is available")

	if Target.skipChecks then
		Pass("Skipped")
		Compiler.as = gcc
		return
	end

	if not CreateGasTestFile(tmpName .. ".s") then Error() end

	RunCommand (
		gcc .. " " .. tmpName .. ".s "
	)

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".s")
		Fail("No")
	end

	os.remove(tmpName .. ".s")
	Compiler.as = gcc
end

function CheckWasmAssembler(as, tmpName)
	local wasm = as or "wasm"

	Check("Checking " .. wasm .. " is available")

	if Target.skipChecks then
		Compiler.as = "wasm"
		Pass("Skipped")
		return
	end

	if not CreateMasmTestFile(tmpName .. ".asm") then Error() end

	RunCommand (
		wasm .. " -bt=dos " ..
		tmpName .. ".asm"
	)

	os.remove(tmpName .. ".asm")

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes") else Fail("No") end

	Compiler.as = wasm
end
