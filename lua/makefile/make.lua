local objroot

function PrintFooterHelper()
	if Target.makefile == "djgpp" then print("Done!") end
end

local function GenerateMakefileRules(sourceType)
	local str
	if sourceType.dos then
		str = [[
TARGETS = $(STAT_LIB)

all: $(PKT_STUB) $(OBJPATH)cflags.h $(TARGETS)
	@echo "All done"

$(STAT_LIB): $(OBJS) $(LIB_ARGS)
	$(AR) $@ @$(LIB_ARGS)

$(OBJPATH)%.o: %.c $(C_ARGS)
	$(CC) @$(C_ARGS) -o $@ $<

$(OBJPATH)%.o: %.S $(C_ARGS)
	$(CC) -E @$(C_ARGS) $< > $(OBJPATH)$*.iS
	$(AS) $(AFLAGS) $(OBJPATH)$*.iS -o $@

$(OBJPATH)cpumodel.o: cpumodel.S
]]

		if sourceType.m32 then
			local nasmRules =
[[

$(OBJPATH)pcpkt.obj: asmpkt.nas

$(PKT_STUB): asmpkt.nas
	$(NASM) -f bin -l asmpkt.lst -o asmpkt.bin asmpkt.nas
	$(LUA) $(LUAPATH)bin2c.lua asmpkt.bin > $@
]]
			if Compiler.nasm then
				str = str .. "\nNASM=" .. Compiler.nasm .. "\n" .. nasmRules
			else
				str = str .. "\n#NASM=Unknown (USE_FAST_PKT unavailable)\n" .. nasmRules:gsub("([^\r\n]*)([\r\n]*)", '#' .. "%1%2")
			end
		elseif sourceType.win then
			str = str .. [[

$(IMP_LIB): $(WATT_DLL)
	@%null

$(WATT_DLL): $(OBJS) $(RESOURCE) $(LINK_ARGS)
	*$(LD) $(LDFLAGS) name $^@ @$(LINK_ARGS)

DEBUGRC = 0

# TODO: Test for wrc in configur.lua

$(RESOURCE): watt-32.rc
	*wrc -q -bt=nt -dDEBUG=0 -D__WATCOMC__ -r -zm -fo=$^@ $<
]]
		end
	end

	str = str .. [[

$(OBJPATH)cflags.h: $(MAKEFILE_LIST)
	$(LUA) $(LUAPATH)resfile.lua $(MAKEFILE_LIST) CFLAGS "const char *w32_cflags = \"~\";" > $(OBJPATH)cflags.h
	$(LUA) $(LUAPATH)resfile.lua $(MAKEFILE_LIST) CC "const char *w32_cc = \"~\";" >> $(OBJPATH)cflags.h
]]

	return str
end

local function GenerateDjgpp()
	Check("Generating '" .. Target.makefile .. "'")

	-- Create build directory
	objroot = SanitizePath("src/build/djgpp")
	mkdir(objroot)

	-- Create makefile
	local dir = SanitizePath("src/djgpp.mak")
	local file = io.open(dir, "w")
	dir = nil
	if not file then Error() end

	local tag = {asm = true, dos = true, m32 = true}
	local cflags = [[-O3 -g -I. -I../inc -DWATT32_BUILD -W -Wall -Wno-strict-aliasing -march=i386 -mtune=i586]]
	if Compiler.colorOption then cflags = cflags .. " -fdiagnostics-color=never" end
	if tonumber(Compiler.version:match("%d+")) >= 5 then cflags = cflags .. " -fgnu89-inline" end

	file:write(
		MakefileHeader() .. [[
# MAKEFILE_LIST is not populated on GNU Make < v3.80
ifeq ($(origin MAKEFILE_LIST), undefined)
# Make sure this variable correlates to this makefile filename
MAKEFILE_LIST = djgpp.mak
endif
]] .. '\n' ..
		GenerateConfigurables(tag, Compiler.cc, cflags, "", "") .. '\n' ..
		GeneratePaths(string.sub(objroot, string.find(objroot, System.divider) + 1)) .. '\n' ..
		"# Output library\nSTAT_LIB = $(LIBPATH)libwatt.a\n\n" ..
		GenerateSources(tag) .. '\n' ..
		GenerateObjects(tag) .. '\n' ..
		GenerateMakefileRules(tag)
	)
	file:close()
end

function GenerateMakefile()
	if Target.makefile == "djgpp" then GenerateDjgpp() end
end
