require("lua.util")

function CheckAndRemoveCommonArtifacts(baseName)
	local names = {
		baseName,
		"a", -- Typical of GCC when no output name is given
	}

	local extensions = {
	".a",
	".com",
	".dll",
	".exe",
	".lib",
	".o",
	".obj",
	".out",
	"", -- No extension, typical of GCC on Linux
	}

	r = 0

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				io.close(file)
				os.remove(fileName)
				r = r + 1
			end
		end
	end

	return r
end

function CheckCustomCompiler(cc, tmpName)
	Check("Checking CC compiler works")

	if Target.SkipCompilerCheck then
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		return
	end

	RunCommand (
		cc ..
		" " ..
		tmpName ..
		".c"
	)

	if CheckAndRemoveCommonArtifacts(tmpName) then Pass("Yes") else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = cc

	local cflags = CheckEnvVar("CFLAGS")
	if cflags then
		os.remove(tmpName)
		Check("Checking if C compiler works with CFLAGS")
		local r = RunCommand (
			cc ..
			" " ..
			cflags ..
			" " ..
			tmpName ..
			".c"
		)

		if CheckAndRemoveCommonArtifacts(tmpName) then Pass("Yes") else
			os.remove(tmpName .. ".c")
			Fail("No")
		end

		Compiler.cflags = cflags
	end

	local ld = CheckEnvVar("LD")

	return Compiler
end

function CheckGccCompiler(cc, tmpName)
	local gcc = cc or "gcc"

	Compiler.type = "gcc"
	Compiler.output = "-o "
	Compiler.ld = "ld"

	Check("Checking '" .. gcc .. "' is available")

	if Target.SkipCompilerCheck then
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		Compiler.cc = gcc
		return
	end

	RunCommand (
		gcc ..
		" " ..
		tmpName .. ".c"
	)

	exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = gcc

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
	else print("No") end
end

function CheckWatcomCompiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ld = "wlink"

	if Target.SkipCompilerCheck then
		Check("Checking a Open Watcom C compiler is available")
		Compiler.cc = {"wcc", "wcc386"}
		os.remove(tmpName .. ".c")
		Pass("Skipped")
		return
	end

	Compiler.cc = {}

	Check("Checking wcc is available")

	RunCommand (
		"wcc " ..
		tmpName .. ".c"
	)

	local wcc = CheckAndRemoveCommonArtifacts(tmpName)
	if wcc > 0 then
		Pass("Yes")
		table.insert(Compiler.cc, "wcc")
	else print("No") end

	Check("Checking wcc386 is available")
	RunCommand (
		"wcc386 " ..
		tmpName .. ".c"
	)

	local wcc386 = CheckAndRemoveCommonArtifacts(tmpName)
	if wcc386 > 0 then
		Pass("Yes")
		table.insert(Compiler.cc, "wcc386")
	else print("No") end

	Check("Checking a Open Watcom C compiler is available")
	if wcc > 0 or wcc386 > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end
end
