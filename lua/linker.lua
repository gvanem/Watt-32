require("lua.util")

function CheckWlinkLinker()
	Check("Checking wlink works")
	local tmpName = UniqueName()

	if not CreateCTestFile(tmpName .. ".c") then Error() end

	RunCommand (
		Compiler.cc .. " " ..
		Compiler.output .. tmpName .. ".o "  ..
		tmpName .. ".c"
	)

	local file = io.open(tmpName .. ".o")
	if not file then Error() end
	file:close()

	RunCommand(Compiler.ld .. " f " .. tmpName .. ".o")
	os.remove(tmpName .. ".c")

	local exists = CheckAndReturnCommonExecutable(tmpName)
	CheckAndRemoveCommonArtifacts(tmpName)

	if exists then Pass("Yes") else Fail("No") end
end

function CheckLdLinker()
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
