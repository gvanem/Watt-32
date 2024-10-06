--[[
	compiler.lua contains functions common to testing all C compilers.
]]

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

	local file = io.open(tmpName .. ".txt", "r")
	if file then
		local txt = file:read()
		file:close()
		os.remove(tmpName .. ".txt")
		if txt and txt:gsub("%s+$", "") == "Hello World" then
			Pass("Yes")
			CheckAndRemoveCommonArtifacts(tmpName)
			return
		end
	end

	Pass("No")
	Target.xcom = true
	CheckAndRemoveCommonArtifacts(tmpName)
	require("lua.emu")
	Check("Getting full working directory")
	System.wd = GetWorkingDirectory()
	if not System.wd then Fail("Unknown") end
	Pass(System.wd)
	CheckDosEmu()
end

function CheckCompilerIntSize()
	Check("Checking actual size of 'int' C type in bits")

	if Target.xcom or Target.skip then
		-- TODO: Use an emulator to run if xcom true and skip false
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
		-- TODO: Use an emulator to run if xcom true and skip false
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
