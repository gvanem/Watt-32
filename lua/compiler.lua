require("lua.util")

function CreateCTestFile(name, src)
	if not src then src = "int main(void) {\n\treturn 0;\n}\n" end

	local file = io.open(name, "w")
	if file then
		file:write(src)
		file:close()
		return true
	end

	return false
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
	Check("Checking actual size of 'int' C type in bits")

	if Target.crossCompile or Target.skipChecks then
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
	Check("Checking actual size of 'long' C type in bits")

	if Target.crossCompile or Target.skipChecks then
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
