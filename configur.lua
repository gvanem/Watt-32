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
	"compiler/wcc.lua",
	"makefile.lua",
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
CheckCompilerIntSize()
CheckCompilerLongSize()

-- Check linker works
CheckLinker()

-- Generate makefile
GenerateMakefile()

-- TODO: Print valid makefile commands the same as 'configur(.bat/.sh)'

Check("Checking script is still work-in-progress")
Fail("Yes, thanks for testing. Please provide feedback.")

os.exit()
