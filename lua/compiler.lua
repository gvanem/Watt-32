require("lua.util")

function CheckAndReturnCommonExecutable(baseName)
	local names = {
		baseName,
		"a", -- Typical of GCC when no output name is given
	}

	local extensions = {
	".com",
	".exe",
	".out",
	"", -- No extension, typical of GCC on Linux
	}

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				io.close(file)
				return fileName
			end
		end
	end
end

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

	local r = 0

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

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = gcc
	Compiler.cl = gcc

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

function CheckWccCompiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ld = "wlink"

	Check("Checking wcc is available")

	if Target.SkipCompilerCheck then
		Compiler.cc = "wcc"
		Compiler.cl = "wcl"
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

	Compiler.cc = "wcc"
	Compiler.cl = "wcl -q"
end

function CheckWcc386Compiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ld = "wlink"

	Check("Checking wcc386 is available")

	if Target.SkipCompilerCheck then
		Compiler.cc = "wcc386"
		Compiler.cl = "wcl386"
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
end

function GetBitSizeResult(fileName)
	file = io.open(fileName)
	if not file then return 0 end

	local result = file:read("*a")
	io.close(file)

	if result == "65535\n" then return 16
	elseif result == "4294967295\n" then return 32
	elseif result == "18446744073709551615\n" then return 64
	else return 0 end
end

function CheckCompilerIntSize()
	Check("Checking size of 'int' C type in bits")

	if Target.SkipCompilerCheck then
		Pass("Skipped")
		return
	end

	local tmpName = UniqueName()
	local file = io.open(tmpName .. ".c", "w")

	file:write(
[[
#include <stdio.h>
int main(void) {
	unsigned int i = -1;
	printf("%u\n", i);
	return 0;
}
]]
	)

	io.close(file)

	RunCommand (
		Compiler.cl .. " " ..
		tmpName .. ".c"
	)
	os.remove(tmpName .. ".c")

	local bin = CheckAndReturnCommonExecutable(tmpName)
	if not bin then Error() end

	RunCommandLocal(bin .. " > " .. tmpName .. ".txt")
	os.remove(bin)

	local result = GetBitSizeResult(tmpName .. ".txt")
	os.remove(tmpName .. ".txt")

	CheckAndRemoveCommonArtifacts(tmpName)
	if result == 0 then Error() end

	Pass(result)
	Compiler.int = result
end

function CheckCompilerLongSize()
	Check("Checking size of 'long' C type in bits")

	if Target.SkipCompilerCheck then
		Pass("Skipped")
		return
	end

	local tmpName = UniqueName()
	local file = io.open(tmpName .. ".c", "w")

	file:write(
[[
#include <stdio.h>
int main(void) {
	unsigned long i = -1;
	printf("%lu\n", i);
	return 0;
}
]]
	)

	io.close(file)

	RunCommand (
		Compiler.cl .. " " ..
		tmpName .. ".c"
	)
	os.remove(tmpName .. ".c")

	local bin = CheckAndReturnCommonExecutable(tmpName)
	if not bin then Error() end

	RunCommandLocal(bin .. " > " .. tmpName .. ".txt")
	os.remove(bin)

	local result = GetBitSizeResult(tmpName .. ".txt")
	os.remove(tmpName .. ".txt")

	CheckAndRemoveCommonArtifacts(tmpName)
	if result == 0 then Error() end

	Pass(result)
	Compiler.long = result
end
