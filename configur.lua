require("lua.boot")

-- Determine operating system family
System = CheckSystemFamily()

-- Check 'lua' contains required files
CheckDirContains("lua", {
	"asm.lua",
	"checks.lua",
	"compiler.lua",
	"compiler/custom.lua",
	"compiler/gcc.lua",
	"compiler/watcom.lua",
	"makefile.lua",
	"makefile/make.lua",
	"makefile/wmake.lua",
	"util.lua",
	}
)

-- Required files are found and can now be loaded
require("lua.checks")
require("lua.makefile")
require("lua.util")

Target = {}
GetOpt()

-- Check parameters and which makefile to generate
Target.makefile = CheckMakefileRequestValid()

-- Create a unique name for testing file and folder
TmpFolder = UniqueName()

-- Check creating/deleting files and folders works
CheckCreateDirCmd(TmpFolder)
CheckRemoveFileCmd(TmpFolder)
CheckRemoveDirCmd(TmpFolder)

-- Check 'util' contains required source code files
CheckDirContains("util", {
	"errnos.c",
	}
)

-- Check 'inc' contains required files (for application to include)
CheckDirContains("inc", {
	"arpa/inet.h",
	"netdb.h",
	"netinet/in.h",
	"sys/socket.h",
	"tcp.h",
	}
)

-- Check 'src' contains required files
CheckDirContains("src", MakefileCoreSource())

-- Create a basic C file to test the compiler
Compiler = {}
CheckAssembler()
CheckCompiler()

-- Check size of standard types an actual 32-bit typedef can be defined
CheckCompilerNative() -- Check that these tests can be done (or skipped)
CheckDosEmu()
CheckCompilerIntSize()
CheckCompilerLongSize()

-- Check archiver and linker works
CheckArchiver()
CheckLinker()

-- Free memory (nessasary for real mode DOS systems)
package.loaded["lua.boot"] = nil
package.loaded["lua.checks"] = nil
package.loaded["lua.compiler"] = nil
package.loaded["lua.compiler.custom"] = nil
package.loaded["lua.compiler.gcc"] = nil
package.loaded["lua.compiler.watcom"] = nil
collectgarbage("collect", 9001)

-- Generate makefile
GenerateMakefile()

-- Print helpful commands to run next
PrintFooterHelper()

os.exit()
