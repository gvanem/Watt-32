--[[
	custom.lua contains the functions needed to test
	that the provided cc ld cflags etc all work together
]]

function CheckCompiler(cc, tmpName)
	Check("Checking '" .. cc .. "' compiler works")

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

function CheckLinker(ld, tmpName)
	Check("Checking '" .. ld .."' works")
	Pass("Not yet implemented")
end
