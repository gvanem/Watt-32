local objroot = SanitizePath("src/build/watcom/")

local function GenerateMakefileRules(sourceType)
return [[
TARGETS = $(STAT_LIB)

all: $(PKT_STUB) $(OBJPATH)cflags.h $(OBJPATH)cflagsbf.h $(TARGETS) .SYMBOLIC
	@echo All done

$(STAT_LIB): $(OBJS) $(LIB_ARGS)
	$(AR) -q -b -c -pa -z=export.tmp $^@ @$(LIB_ARGS)

$(OBJPATH)language.obj: language.c lang.c
$(OBJPATH)asmpkt.obj:   asmpkt.asm
$(OBJPATH)cpumodel.obj: cpumodel.asm

.c{$(OBJDIR)}.obj: .AUTODEPEND $(C_ARGS)
	$(CC) @$(C_ARGS) $[@ -fo=$^@

.asm{$(OBJDIR)}.obj: .AUTODEPEND
	$(AS) $(AFLAGS) $[@ -fo=$^@

$(C_ARGS): $(__MAKEFILES__)
	%create $^@
	@%append $^@ $(CFLAGS)

$(LIB_ARGS): $(__MAKEFILES__)
	%create $^@
	@for %f in ($(OBJS)) do @%append $^@ +- %f

$(OBJPATH)cflags.h: $(__MAKEFILES__)
	%create $^@
	@%append $^@ const char *w32_cflags = "$(CFLAGS)";
	@%append $^@ const char *w32_cc     = "$(CC)";

$(OBJPATH)cflagsbf.h: $(C_ARGS)
	$(LUA) $(LUAPATH)bin2c.lua $(C_ARGS) > $^@
]]
end

local function MakeBuildFiles(header, makefile, objPath, cc, cflags, aflags, statlib, extra)
	Check("Generating '" .. makefile .. "'")
	-- Create build directory
	local objdir = SanitizePath(objroot .. objPath)
	mkdir(objdir)

	-- Create makefile
	local dir = SanitizePath("src/" .. makefile)
	local file = io.open(dir, "w")
	if not file then Error() end

	-- Generate a 'tag' string so that only required sources and objects are added to the makefile
	local tag = { bind = true, bsd = true, core = true }

	if objPath == "win32" then
		tag.win = true
		tag.link = true
		if not Compiler.ld then Compiler.ld = "wlink" end
		if not Compiler.ldflags then Compiler.ldflags = "system nt_dll" end
		extra =
[[

# Win32 specifics
IMP_LIB  = $(LIBPATH)wattcpww_imp.lib
WATT_DLL = $(BINPATH)watt-32.dll
RESOURCE = $(OBJPATH)watt-32.re
]]
	else
		tag.asm = true
		tag.dos = true
	end

	if not header then header = MakefileHeader() end
	if not extra then extra = "" end

	file:write(
		header .. '\n' ..
		GenerateConfigurables(tag, cc, cflags, aflags) .. '\n' ..
		GeneratePaths(string.sub(objdir, string.find(objdir, System.divider) + 1)) .. '\n' ..
		"# Output library\nSTAT_LIB = $(LIBPATH)" .. statlib .. '\n' ..
		extra .. '\n' ..
		GenerateSources(tag) .. '\n' ..
		GenerateObjects(tag) .. '\n' ..
		GenerateMakefileRules(tag)
	)
	file:close()
	Pass("Done")
end

local function GenerateErrorFile()
	local cErrFilePath = objroot .. "syserr.c"
	local hErrFilePath = SanitizePath("inc/sys/watcom.err")
	Check("Generating syserr files for Watcom")

	-- Do nothing if syserr.c and watcom.err already exists
	if FileExists(cErrFilePath) and FileExists(hErrFilePath) then Pass("Skipped") return end

	local headerPath = SanitizePath("$(%WATCOM)/h")
	local sourcePath = SanitizePath("util/errnos.c")
	local targetPath = SanitizePath("util/wc_err.exe")

	-- Choose between wcl or wcl386
	if Compiler.cl16 then
		RunCommand(Compiler.cl16 .. [[-zq -bcl=dos -ml -wx -I"inc" -I"]] .. headerPath .. [[" -fe=]] .. targetPath .. ' ' .. sourcePath)
	elseif Compiler.cl then
		RunCommand(Compiler.cl .. [[-zq -bcl=dos4g -mf -wx -I"inc" -I"]] .. headerPath .. [[" -fe=]] .. targetPath .. ' ' .. sourcePath)
	else
		Fail("No compiler/linker set")
	end

	if not FileExists(targetPath) then Fail("Failed to compile " .. sourcePath) end

	RunCommandLocal(targetPath .. " -e > " .. hErrFilePath)
	RunCommandLocal(targetPath .. " -s > " .. cErrFilePath)
	Pass("Done")
end

function GenerateMakefile()
	-- Setup enviroment
	mkdir(objroot)
	GenerateErrorFile()

	-- Generate makefiles
	if Compiler.cc16 then
		local aflags = [[-bt=dos -zq -w3 -d1 -I"../inc"]]
		local wccHelp =
[[
#
#  WCC-flags used:
#   -bt=dos   target system - DOS
#   -m{s,l}   memory model; small or large
#   -0        optimise for 8086, register call convention
#   -s        no stack checking
#   -zq       quiet compiling
#   -zc       place const data into the code segment
#   -d{1,2,3} generate debug info
#   -os       optimization flags
#     s:      favour code size over execution time
#
]]

		-- large model (16-bit)
		MakeBuildFiles(
			MakefileHeader() .. "# This makefile builds Watt-32 for large model 16-bit DOS\n" .. wccHelp ,
			"watcom_l.mak",
			"large",
			Compiler.cc16,
			[[-bt=dos -ml -0 -os -s -zc -zm -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			aflags,
			"wattcpwl.lib"
		)

		-- small model (16-bit)
		MakeBuildFiles(
			MakefileHeader() .. "# This makefile builds Watt-32 for small model 16-bit DOS\n" .. wccHelp,
			"watcom_s.mak",
			"small",
			Compiler.cc16,
			[[-bt=dos -ms -0 -os -s -zc -zm -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			aflags,
			"wattcpws.lib"
		)
	end

	if Compiler.cc then
		local wccHelp =
[[
#
# WCC386-flags used:
#   -bt=dos   target system - DOS
#   -bt=nt    target system - Win-NT
#   -m{s,f}   memory model; small or flat
#   -3s       optimise for 386, stack call convention
#   -3r       optimise for 386, register call convention
#   -s        no stack checking
#   -zq       quiet compiling
#   -d{1,2,3} generate debug info
#   -zlf      always generate default library information
#   -zm       place each function in separate segment
#   -oilrtfm  optimization flags
#     i:      expand intrinsics
#     l:      loop optimisations
#     r:      reorder instructions
#     t:      favor execution time
#     f:      always use stack frames
#     m:      generate inline code for math functions
#
]]

		-- small model (32-bit DOS4GW)
		MakeBuildFiles(
			MakefileHeader() .. "# This makefile builds Watt-32 for small model DOS4GW\n" .. wccHelp,
			"watcom_3.mak",
			"small32",
			Compiler.cc,
			[[-bt=dos -ms -3r -oaxt -s -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -3s -dDOSX -dDOS4GW -zq -w3 -d1 -I"../inc"]],
			"wattcpw3.lib"
		)

		-- flat model  (DOS4GW)
		MakeBuildFiles(
			MakefileHeader() .. "# This makefile builds Watt-32 for flat model DOS4GW\n" .. wccHelp,
			"watcom_f.mak",
			"flat",
			Compiler.cc,
			[[-bt=dos -mf -3r -zff -zgf -oilrtfm -s -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=dos -3r -dDOSX -dDOS4GW -zq -w3 -d1 -I"../inc"]],
			"wattcpwf.lib"
		)

		-- Win32
		MakeBuildFiles(
			MakefileHeader() .. "# This makefile builds Watt-32 for Win32\n" .. wccHelp,
			"watcom_w.mak",
			"win32",
			Compiler.cc,
			[[-bt=nt -mf -3r -fp6 -oilrtfm -s -bm -zri -zlf -zq -wx -DWATT32_BUILD -I. -I"../inc" -d1]],
			[[-bt=nt -3s -dDOSX -zq -w3 -d1 -I"../inc"]],
			"wattcpww.lib"
		)
	end
end
