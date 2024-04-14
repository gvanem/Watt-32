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

function CheckCustomCompiler(cc, tmpName)
	Check("Checking CC compiler works")

	if Target.skipChecks then
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

	local exists = CheckAndRemoveCommonArtifacts(tmpName)
	if exists > 0 then Pass("Yes") else
		os.remove(tmpName .. ".c")
		Fail("No")
	end

	Compiler.cc = cc
	Compiler.cl = cc

	local cflags = CheckEnvVar("CFLAGS")
	if cflags then
		Check("Checking if C compiler works with CFLAGS")
		local r = RunCommand (
			cc ..
			" " ..
			cflags ..
			" " ..
			tmpName ..
			".c"
		)

		local exists = CheckAndRemoveCommonArtifacts(tmpName)
		if exists > 0 then Pass("Yes")
		else
			os.remove(tmpName .. ".c")
			Fail("No")
		end

		Compiler.cflags = cflags
	end
end

function CheckGccCompiler(cc, tmpName)
	local gcc = cc or "gcc"

	Compiler.type = "gcc"
	Compiler.output = "-o "
	Compiler.ld = "ld"

	Check("Checking '" .. gcc .. "' is available")

	if Target.skipChecks then
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

	Check("Checking if '" .. gcc .. "' can target i386")
	RunCommand (
		gcc ..
		" -m32 -march=i386 " ..
		tmpName .. ".c"
	)

	exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then
		Pass("Yes")
		Compiler.m32 = true
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
		Compiler.m64 = true
	elseif not Compiler.m32 then Fail("No") -- Need at least one target
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

function CheckWccCompiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ld = "wlink"

	Check("Checking wcc is available")

	if Target.skipChecks then
		Compiler.cc = "wcc"
		Compiler.cl = "wcl"
		Compiler.m16 = true
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
	Compiler.m16 = true
end

function CheckWcc386Compiler(tmpName)
	Compiler.type = "watcom"
	Compiler.output = "-fo="
	Compiler.ld = "wlink"

	Check("Checking wcc386 is available")

	if Target.skipChecks then
		Compiler.cc = "wcc386"
		Compiler.cl = "wcl386"
		Compiler.m32 = true
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
	Compiler.m32 = true
end

function GetBitSizeResult(fileName)
	file = io.open(fileName)
	if not file then return 0 end

	local result = file:read("*a")
	file:close()

	if result == "65535\n" then return 16
	elseif result == "4294967295\n" then return 32
	elseif result == "18446744073709551615\n" then return 64
	else return 0 end
end

function CheckCompilerIntSize()
	Check("Checking size of 'int' C type in bits")

	if Target.skipChecks then
		Compiler.int = Target.makefile == "wcc" and 16 or 32
		Pass("Skipped (assuming " .. Compiler.int ..")")
		return
	end

	local tmpName = UniqueName()
	if not CreateCTestFile(tmpName .. ".c",
[[
#include <stdio.h>
int main(void) {
	unsigned int i = -1;
	printf("%u\n", i);
	return 0;
}
]]
	) then Error() end

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

	if Target.skipChecks then
		Compiler.long = 32
		Pass("Skipped (assuming " .. Compiler.long ..")")
		return
	end

	local tmpName = UniqueName()
	if not CreateCTestFile(tmpName .. ".c",
[[
#include <stdio.h>
int main(void) {
	unsigned long i = -1;
	printf("%lu\n", i);
	return 0;
}
]]
	) then Error() end

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
