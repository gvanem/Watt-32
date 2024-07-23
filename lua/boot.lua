--[[
	boot.lua contains the functions needed for the script to start and
	check its own integrity. CheckSystemfamily() should be called first
	followed by CheckDirContains() on all the lua scripts.
	This will ensure that the users gets a legible "file missing" error
	if instead of an overly verbose exception from require().
	The other functions are either nessacary for those two to
	function or are part of the help printout.
]]

function PrintHelp()
	print([[
configur.lua [TARGET] [OPTIONS]
Configures and generates Watt-32 makefiles regardless of host enviroment using Lua
A platform agnostic alternative to `src/configur.sh` and `SRC\\CONFIGUR.BAT`
[TARGET]:
]] ..
	TargetHelpStr() ..
[[
[OPTIONS]:
]] ..
	OptionHelpStr()
	)

	os.exit(3)
end

function TargetHelpStr()
return [[
	cc     - Handle everything yourself with CC,CFLAGS,LD enviroment variables
	clang  - A GCC compatible alternative compiler based on LLVM
	djggp  - A port of GCC for 80386+ DOS systems
	gcc    - GNU Compiler Collection intended for Posix systems
	mingw  - A port of GCC for Win32 based systems
	wcc    - Open Watcom C/C++ toolchain to build 8086-80286 targets
	wcc386 - Open Watcom C/C++ toolchain to build 80386+ targets
]]
end

function OptionHelpStr()
return [[
	-h, --help, /? - Show this help
	-c, --xcom, /c - Cross compiling, skip native checks
	-s, --skip, /s - Skip all compiler checks
]]
end

function GetOpt()
	for _,a in ipairs(arg) do
		if a == '-h' or a == '/?' or a == "--help" then PrintHelp() end
		if a == '-c' or a == '/c' or a == "--xcom" then Target.xcom = true end
		if a == '-s' or a == '/s' or a == "--skip" then Target.skip = true end
	end
end

function CheckDirContains(dir, files)
	Check("Checking '" .. dir .. "' contains required files")

	for _, file in ipairs(files) do
		local path = SanitizePath(dir .. "/" .. file)

		if not FileExists(path) then Fail("Missing '" .. file .. "'") end
	end

	Pass("Yes")
	return true
end

function CheckMakefileRequestValid()
	-- TODO: Add Borland C and other antiques?
	local options = {
		"cc",
		"clang",
		"djgpp",
		"gcc",
		"mingw",
		"wcc",
		"wcc386",
	}

	for _, a in ipairs(arg) do
		for _, o in ipairs(options) do
			if a == o then return a end
		end
	end

	if os.getenv('CC') then return "cc" end

	print(
		"Specify a makefile to generate, options are:\n" ..
		TargetHelpStr() ..
		"\nUse '/?' or '-h' to get full help"
	)

	os.exit(3)
end

function Check(msg)
	io.write(msg .. "... ")
	io.flush()
end

function Pass(msg)
	print(msg)
end

function Fail(msg)
	print(msg)
	os.exit(1)
end

function Error()
	print("Error!")
	os.exit(2)
end

function SanitizePath(path)
	local sanitize
	if System.family == "Unix" then
		sanitize = path:gsub([[\]], [[/]])
	else
		sanitize = path:gsub([[/]], [[\]])
	end

	return sanitize
end

function CheckSystemFamily()
	local sys = {}

	Check("Determining operating system family")
	-- Every Microsoft operating system since PC-DOS 2 sets %COMSPEC%
	local env = os.getenv("COMSPEC")

	if env then
		sys.divider = "\\"
		sys.md = "MD"
		sys.rm = "DEL"

		-- Every NT operating system sets %OS%
		env = os.getenv("OS")
		if env and env == "Windows_NT" then
			sys.family = "Nt"
			sys.rd = "RD /S /Q"
		else
			sys.family = "Dos"
			sys.rd = "DELTREE /Y"
		end
	else -- Assume it's a Unix system
		sys.divider = "/"
		sys.family = "Unix"
		sys.md = "mkdir"
		sys.rd = "rm -R"
		sys.rm = "rm"
	end

	Pass(sys.family)
	return sys
end

function FileExists(name)
	local file, err = io.open(name)
	if file then
		file:close()
		return true
	end

	-- Directory testing
	if err:match("Is a directory") or err:match("Permission denied") then
		return true
	end

	return false
end
