function PrintHelp()
	print(
"configur.lua [TARGET] [OPTIONS]\n" ..
"Configures and generates Watt-32 makefiles regardless of host enviroment using Lua\n" ..
"A platform agnostic alternative to `src/configur.sh` and `SRC\\CONFIGUR.BAT`\n" ..
"\n[TARGET]:\n" ..
TargetHelpStr() ..
"\n[OPTIONS]:\n" ..
OptionHelpStr()
	)

	os.exit(3)
end

function TargetHelpStr()
return "\tcc\t- Handle everything yourself with CC,CFLAGS,LD enviroment variables\n" ..
"\tclang\t- A GCC compatible alternative compiler based on LLVM\n" ..
"\tdjggp\t- A port of GCC for 80386+ DOS systems\n" ..
"\tgcc\t- GNU Compiler Collection intended for Posix systems\n" ..
"\tmingw\t- A port of GCC for Win32 based systems\n" ..
"\twcc\t- Open Watcom C/C++ toolchain to build 8086-80286 targets\n" ..
"\twcc386\t- Open Watcom C/C++ toolchain to build 80386+ targets\n"
end

function OptionHelpStr()
return "\t-h, --help, /? - Show this help\n" ..
"\t-s, /s         - Skip compiler checks\n"
end

function GetOpt()
	local help = { "-h", "--help", "/?" }
	for _,a in ipairs(arg) do
		for _,h in ipairs(help) do
			if a == h then
				PrintHelp()
			end
		end

		if a == '-s' or a == '/s' then Target.SkipCompilerCheck = true end
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
