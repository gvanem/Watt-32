local objroot = "src/build/watcom/"

local function StringToHexArray(str)
	local hexArray = {}

	for i = 1, #str do
		hexArray[#hexArray + 1] = string.format("0x%02X", string.byte(str, i))
	end

	return table.concat(hexArray, ", ")
end

local function MakeBuildFilesLarge()
	local objdir = SanitizePath(objroot .. "large")
	mkdir(objdir)
	local cflags = "-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC"

	local dir = SanitizePath(objdir .. "/cflags.h")
	Check("Writing cflags to '" .. dir .. "'")
	local file = io.open(dir, "w")
	if not file then Error() end
	file:write([[
const char *w32_cflags = "]] .. cflags .. [[";
const char *w32_cc     = "*]] .. Compiler.cc16 .. [[";
]]
	)
	file:close()
	Pass("Done")

	dir = SanitizePath(objdir .. "/cflagsbf.h")
	Check("Writing cflags hex array to '" .. dir .. "'")
	file = io.open(dir, "w")
	if not file then Error() end
	file:write(StringToHexArray(cflags))
	file:close()
	Pass("Done")
end

local function WMake_Asm()
	return [[
AS = *]] .. Compiler.as or "wasm" .. [[
AR = *]] .. Compiler.ld or "wlib" .. [[

]]
end

local function WMake_3()
	return [[
CC       = *]] .. Compiler.cc or "wcc386" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=dos -ms -3r -oaxt -s -zlf -DWATT32_STATIC" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=dos -3s -dDOSX -dDOS4GW" .. [[
STAT_LIB = $(LIBPATH)wattcpw3.lib
OBJDIR   = $(OBJROOT)small32
]]
end

local function WMake_F()
	return [[
CC       = *]] .. Compiler.cc or "wcc386" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=dos -3r -dDOSX -dDOS4GW" .. [[
STAT_LIB = $(LIBPATH)wattcpwf.lib
OBJDIR   = $(OBJROOT)flat
]]
end

local function WMake_L()
	return [[
CC       = *]] .. Compiler.cc16 or "wcc" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=dos -ml -0 -os -s -zc -zm -zlf -DWATT32_STATIC" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=dos" .. [[
STAT_LIB = $(LIBPATH)wattcpwl.lib
OBJDIR   = $(OBJROOT)large
]]
end

local function WMake_S()
	return [[
CC       = *]] .. Compiler.cc16 or "wcc" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=dos -ms -0 -os -s -zc -zm -zlf -DWATT32_STATIC" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=dos" .. [[
STAT_LIB = $(LIBPATH)wattcpws.lib
OBJDIR   = $(OBJROOT)small
]]
end

local function WMake_W()
	return [[
CC       = *]] .. Compiler.cc or "wcc386" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=nt -mf -3r -fp6 -oilrtfm -s -bm -zri -zlf" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=nt -3s -dDOSX" .. [[
LDFLAGS  = system nt_dll
STAT_LIB = $(LIBPATH)wattcpww.lib
IMP_LIB  = $(LIBPATH)wattcpww_imp.lib
WATT_DLL = $(DLLPATH)watt-32.dll
OBJDIR   = $(OBJROOT)win32
RESOURCE = $(OBJPATH)watt-32.res
]]
end

local function WMake_X()
	return [[
CC       = *]] .. Compiler.cc or "wcc386" .. [[
CFLAGS   = ]] .. Compiler.cflags or "-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC" .. [[
AFLAGS   = ]] .. Compiler.aflags or "-bt=dos -3r -dDOSX -dDOS4GW" .. [[
STAT_LIB = $(LIBPATH)wattcpwf.lib
OBJDIR   = $(OBJROOT)flat
]]
end

function GenerateMakefile()
	MakeBuildFilesLarge()
end
