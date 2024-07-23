--[[
	makefile.lua contains functions for generating makefiles
]]

function MakefileHeader()
	return
[[
#
# NB! THIS MAKEFILE WAS AUTOMATICALLY GENERATED.
# If you make manual edits, be sure to save them as a different name
# otherwise your changes might be overwriten.
#
# Consider running "<lua> configur.lua <target>" instead of editing
#
]]
end

function TableStringFormat(t, p, a)
	if p then
		for i, v in pairs(t) do
			t[i] = p .. v
		end
	end

	if a then
		for i, v in pairs(t) do
			t[i] = v .. a
		end
	end
end

function MakefileAssembly(prepend, append)
	local assembly = {"asmpkt", "cpumodel"}
	TableStringFormat(assembly, prepend, append)
	return assembly
end

function MakefileBindSource()
	return {
	"res_comp.c", "res_data.c", "res_debu.c", "res_init.c",
	"res_loc.c", "res_mkqu.c", "res_quer.c", "res_send.c",
	}
end

function MakefileBsdSource()
	return {
	"accept.c", "bind.c", "bsddbug.c", "close.c", "connect.c",
	"fcntl.c", "fsext.c", "get_ai.c", "get_ip.c", "get_ni.c",
	"get_xbyr.c", "geteth.c", "gethost.c", "gethost6.c", "getname.c",
	"getnet.c", "getprot.c", "getput.c", "getserv.c", "ioctl.c",
	"linkaddr.c", "listen.c", "netaddr.c", "neterr.c", "nettime.c",
	"nsapaddr.c", "presaddr.c", "printk.c", "receive.c", "select.c",
	"shutdown.c", "signal.c", "socket.c", "sockopt.c", "stream.c",
	"syslog.c", "syslog2.c", "transmit.c",
	}
end

function MakefileCoreSource()
	return {
	"bsdname.c", "btree.c", "chksum.c", "country.c", "crc.c",
	"dynip.c", "echo.c", "getopt.c", "gettod.c", "highc.c", "idna.c",
	"ip4_frag.c", "ip4_in.c", "ip4_out.c", "ip6_in.c", "ip6_out.c",
	"language.c", "lookup.c", "loopback.c", "misc.c", "netback.c",
	"oldstuff.c", "packet32.c", "pc_cbrk.c", "pcarp.c", "pcbootp.c",
	"pcbuf.c", "pcconfig.c", "pcdbug.c", "pcdhcp.c", "pcdns.c",
	"pcicmp.c", "pcicmp6.c", "pcigmp.c", "pcintr.c", "pcping.c",
	"pcpkt.c", "pcpkt32.c", "pcqueue.c", "pcrarp.c", "pcrecv.c",
	"pcsed.c", "pcstat.c", "pctcp.c", "ports.c", "powerpak.c",
	"ppp.c", "pppoe.c", "profile.c", "punycode.c", "qmsg.c", "run.c",
	"settod.c", "sock_dbu.c", "sock_in.c", "sock_ini.c", "sock_io.c",
	"sock_prn.c", "sock_scn.c", "sock_sel.c", "split.c", "misc_str.c",
	"swsvpkt.c", "tcp_fsm.c", "tcp_md5.c", "tftp.c", "timer.c",
	"udp_rev.c", "version.c", "wdpmi.c", "win_dll.c", "winadinf.c",
	"winmisc.c", "winpkt.c", "x32vm.c",
	}
end

function MakefileZlibSource()
	return {
	"zadler32.c", "zcompres.c", "zcrc32.c", "zgzio.c",
	"zuncompr.c", "zdeflate.c", "ztrees.c", "zutil.c", "zinflate.c",
	"zinfback.c", "zinftree.c", "zinffast.c",
	}
end

function MakefileCommon(prepend, append)
	local common = {
	"cpumodel", "accept", "bind", "bsddbug", "bsdname", "btree",
	"chksum", "close", "connect", "crc", "dynip", "echo", "fcntl",
	"get_ai", "get_ip", "get_ni", "get_xbyr", "geteth", "gethost",
	"gethost6", "getname", "getnet", "getopt", "getprot", "getput",
	"getserv", "gettod", "idna", "ioctl", "ip4_frag", "ip4_in",
	"ip4_out", "ip6_in", "ip6_out", "language", "linkaddr", "listen",
	"lookup", "loopback", "misc", "netaddr", "netback", "neterr",
	"nettime", "nsapaddr", "oldstuff", "packet32", "pc_cbrk", "pcarp",
	"pcbootp", "pcbuf", "pcconfig", "pcdbug", "pcdhcp", "pcdns",
	"pcicmp", "pcicmp6", "pcigmp", "pcping", "pcqueue", "pcrarp",
	"pcrecv", "pcsed", "pcstat", "pctcp", "ports", "ppp", "pppoe",
	"presaddr", "printk", "profile", "punycode", "receive", "res_comp",
	"res_data", "res_debu", "res_init", "res_loc", "res_mkqu",
	"res_quer", "res_send", "run", "select", "settod", "shutdown",
	"signal", "sock_dbu", "sock_in", "sock_ini", "sock_io", "sock_prn",
	"sock_scn", "sock_sel", "socket", "sockopt", "split", "stream",
	"misc_str", "swsvpkt", "syslog", "syslog2", "tcp_fsm", "tcp_md5",
	"tftp", "timer", "transmit", "udp_rev", "version", "zadler32",
	"zcompres", "zcrc32", "zdeflate", "zgzio", "zinfback", "zinffast",
	"zinflate", "zinftree", "ztrees", "zuncompr", "zutil",
	}

	TableStringFormat(common, prepend, append)
	return common
end

function MakefileAllDosObjects(prepend, append)
	local common = MakefileCommon(prepend, append)
	local dos = {
	"asmpkt", "fsext", "pcpkt32", "pcpkt", "pcintr", "powerpak", "qmsg",
	"wdpmi", "x32vm"
	}
	TableStringFormat(dos, prepend, append)

	table.move(dos, 1, #dos, #common + 1, common)
	return common
end

function MakefileCreateVariable(name, value)
	if type(value) == "table" then
		return name .. " = " .. table.concat(value, " ")
	else
		return name .. " = " .. value
	end
end

function GenerateAsmSources()
	return MakefileCreateVariable("ASM_SOURCE",
		MakefileAssembly(nil, Compiler.aext)
	)
end

function GenerateBindSources()
	return MakefileCreateVariable("BIND_SOURCE", MakefileBindSource())
end

function GenerateBsdSources()
	return MakefileCreateVariable("BSD_SOURCE", MakefileBsdSource())
end

function GenerateCoreSources()
	return MakefileCreateVariable("CORE_SOURCE", MakefileCoreSource())
end

function GenerateZlibSources()
	return MakefileCreateVariable("ZLIB_SOURCE", MakefileZlibSource())
end

function GetMakefileOutputName()
	return "src" .. System.divider .. Target.makefile .. ".mak"
end

function GenerateMakefile()
	if Target.makefile == "wcc" or Target.makefile == "wcc386" then
		require("lua.makefile.watcom")
		GenerateMakefile()
	end
end

--[[
function GenerateMakefile()
	local fileName = GetMakefileOutputName()
	Check("Generating makefile '" .. fileName .. "'")

	local file = io.open(fileName, "w")
	if not file then Error() end

	file:write(MakefileHeader() .. "\n")
	file:write(GenerateAsmSources() .. "\n")
	file:write(GenerateBindSources() .. "\n")
	file:write(GenerateBsdSources() .. "\n")
	file:write(GenerateCoreSources() .. "\n")
	file:write(GenerateZlibSources() .. "\n")

	if Target.makefile == "wcc" or Target.makefile == "wcc386" then
		GenerateMakefileWatcom(file)
	else
		GenerateMakefileUnix(file)
	end

	file:close()
	Pass("Done")
end
--]]
function GenerateMakefileUnix(file)
	-- TODO: write a unix style makefile depending on -m32 or -m64
	file:write([[# A Unix makefile would go here in the completed version]])
end

function GenerateCleanRule(path)
	return "clean:\n" ..
	"\t" .. System.rd .. " " .. path .. "\n\n"
end

function GenerateMakefileWatcomLarge()
	-- TODO: Temporary function, rewrite to handle all memory types
	return [[
$(STAT_LIB): $(OBJS) $(LIB_ARGS)
	*wlib -q -b -c -pa -z=export.tmp $^@ @$(LIB_ARGS)

$(OBJPATH)language.obj: language.c lang.c
$(OBJPATH)asmpkt.obj:   asmpkt.asm
$(OBJPATH)cpumodel.obj: cpumodel.asm

.c{$(OBJDIR)}.obj: .AUTODEPEND
	*]].. Compiler.cc .. " " .. Compiler.cflags .. [[ $[@ -fo=$^@

.asm{$(OBJDIR)}.obj: .AUTODEPEND
	*wasm ]] .. Compiler.aflags .. [[ $[@ -fo=$^@

]]
end

function GenerateMakefileWatcom(file)
	-- TODO: Setup parameters so it can be used to write all memory models not just large
	local binPath = SanitizePath("../bin/")
	local libPath = SanitizePath("../lib/")
	local libName = "wattcpwl.lib"
	local objPath = SanitizePath("build/watcom/large/")

	if not Compiler.aflags then
		Compiler.aflags = [[-bt=dos -zq -w3 -d1 -I"../inc"]]
	end

	if not Compiler.cflags then
		Compiler.cflags = [[-bt=dos -ml -0 -os -s -zc -zm -zlf -DWATT32_STATIC -zq -wx -DWATT32_BUILD -I. -I"../inc" -I"$(%WATCOM)/h"]]
	end

	file:write(
		MakefileCreateVariable("OBJS", MakefileAllDosObjects([[$(OBJPATH)]], ".obj")) .. "\n" ..
		MakefileCreateVariable("BINPATH", binPath) .. "\n" ..
		MakefileCreateVariable("LIBPATH", libPath) .. "\n" ..
		MakefileCreateVariable("OBJPATH", objPath) .. "\n" ..
		MakefileCreateVariable("STAT_LIB", [[$(LIBPATH)]] .. libName) .. "\n\n" ..
		GenerateMakefileWatcomLarge() ..
		GenerateCleanRule(objPath)
	)
end
