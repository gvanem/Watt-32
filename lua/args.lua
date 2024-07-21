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
