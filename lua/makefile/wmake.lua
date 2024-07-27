local objroot = "src/build/watcom/"

local function StringToHexArray(str)
	local hexArray = {}

	for i = 1, #str do
		hexArray[#hexArray + 1] = string.format("0x%02X", string.byte(str, i))
	end

	return table.concat(hexArray, ", ")
end

local function MakeBuildFiles(makefile, objPath, cc, cflags, aflags, statlib)
	Check("Generating '" .. makefile .. "'")
	local objdir = SanitizePath(objroot .. objPath)
	mkdir(objdir)

	-- Create cflags.h
	local dir = SanitizePath(objdir .. "/cflags.h")
	local file = io.open(dir, "w")
	if not file then Error() end
	file:write([[
const char *w32_cflags = "]] .. cflags .. [[";
const char *w32_cc     = "*]] .. cc .. [[";
]]
	)
	file:close()

	-- Create cflagsbf.h (hex array of cflag string
	dir = SanitizePath(objdir .. "/cflagsbf.h")
	file = io.open(dir, "w")
	if not file then Error() end
	file:write(StringToHexArray(cflags))
	file:close()

	-- Create makefile
	dir = SanitizePath("src/" .. makefile)
	file = io.open(dir, "w")
	if not file then Error() end

	-- Generate a 'tag' string so that only required sources and objects are added to the makefile
	local tag = "bindbsdcore"
	if objPath == "large" or objPath == "flat" then
		tag = tag .. "asmdos"
	elseif objPath == "win32" then
		tag = tag .. "win"
	end

	file:write(
		MakefileHeader() ..
		GenerateSources(tag) ..
		"\nBINPATH = " .. SanitizePath("../bin/") .. '\n' ..
		"LIBPATH = " .. SanitizePath("../lib/") .. '\n' ..
		"LUAPATH = " .. SanitizePath("../lua/") .. '\n' ..
		"\nOBJPATH = " .. SanitizePath(objdir .. "/") .. '\n' ..
		GenerateObjects(tag) ..
		"\nAS = " .. Compiler.as .. "\n" ..
		"AR = " .. Compiler.ar .. "\n" ..
		"CC = " .. cc .. "\n" ..
		"LUA = " .. System.lua .. '\n' ..
		"\nCFLAGS = " .. cflags .. "\n" ..
		"AFLAGS = " .. aflags .. "\n" ..
		"\nSTAT_LIB = $(LIBPATH)" .. statlib .. "\n"
	)
	file:close()
	Pass("Done")
end

function GenerateMakefile()
	if Compiler.cc16 then
		-- large model (16-bit)
		MakeBuildFiles(
			"watcom_l.mak",
			"large",
			Compiler.cc16,
			[[-bt=dos -ml -0 -os -s -zc -zm -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -zq -w3 -d1 -I"../inc"]],
			"wattcpwl.lib"
		)

		-- small model (16-bit)
		MakeBuildFiles(
			"watcom_s.mak",
			"small",
			Compiler.cc16,
			[[-bt=dos -ms -0 -os -s -zc -zm -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -zq -w3 -d1 -I"../inc"]],
			"wattcpws.lib"
		)
	end

	if Compiler.cc then
		-- small model (32-bit DOS4GW)
		MakeBuildFiles(
			"watcom_3.mak",
			"small32",
			Compiler.cc,
			[[-bt=dos -ms -3r -oaxt -s -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -3r -dDOSX -dDOS4GW -zq -w3 -d1 -I"../inc"]],
			"wattcpw3.lib"
		)

		-- flat model  (DOS4GW)
		MakeBuildFiles(
			"watcom_f.mak",
			"flat",
			Compiler.cc,
			[[-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -3r -dDOSX -dDOS4GW -zq -w3 -d1 -I"../inc"]],
			"wattcpwf.lib"
		)

		-- Win32
		MakeBuildFiles(
			"watcom_w.mak",
			"win32",
			Compiler.cc,
			[[-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -3r -dDOSX -dDOS4GW -zq -w3 -d1 -I"../inc"]],
			"wattcpww.lib"
		)
	end
end
