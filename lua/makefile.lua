require("lua.util")

function MakefileHeader()
	return 
[[
#
# NB! THIS MAKEFILE WAS AUTOMATICALLY GENERATED.
#     Consider running "<lua> configur.lua <target>" instead of editing
#
# Makefile for the Watt-32 TCP/IP stack.
#
]]
end

function MakefileAsmSource()
	return {
	"asmpkt.asm", "cpumodel.asm",
	}
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

function MakefileCreateVariable(name, value)
	return name .. " = " .. table.concat(value, " ")
end

function GenerateAsmSources()
	return MakefileCreateVariable("ASM_SOURCE", MakefileAsmSource())
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

function GenerateMakefileUnix(file)
	-- TODO: write a unix style makefile depending on -m32 or -m64
	file:write([[# A Unix makefile would go here in the complete version]])
end

function GenerateMakefileWatcom(file)
	-- TODO: write a wmake style makefile depending on wcc or wcc386
	file:write([[# A Watcom makefile would go here in the complete version]])
end
