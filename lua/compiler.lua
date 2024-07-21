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

	if Target.skip then
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

function CheckCompilerNative()
	Check("Checking if '" .. Compiler.cl .. "' builds native binaries")
	if Target.skip then
		Pass("Skipped (assuming no)")
		return
	end

	local tmpName = UniqueName()
	if not CreateCTestFile(tmpName .. ".c",
[[
#include <stdio.h>
int main(void) {
	puts("Hello World");
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

	if FileExists(tmpName .. ".txt") then
		Pass("Yes")
		os.remove(tmpName .. ".txt")
	else
		Pass("No")
		Target.xcom = true
	end
end

function CheckCompilerIntSize()
	Check("Checking actual size of 'int' C type in bits")

	if Target.xcom or Target.skip then
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

	if Target.xcom or Target.skip then
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
