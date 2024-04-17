require("lua.args")
require("lua.checks")
require("lua.makefile")
require("lua.util")

-- Check parameters and which makefile to generate
Target = {}
Target.makefile = CheckMakefileRequestValid()
GetOpt()

-- Can the system acheve basic things?
System = {}
System.family = CheckSystemFamily()
TmpFolder = UniqueName()

System.divider = System.family == "Unix" and "/" or "\\"
System.md = CheckCreateDirCmd(TmpFolder)
System.rm = CheckRemoveFileCmd(TmpFolder)
System.rd = CheckRemoveDirCmd(TmpFolder)

-- Does the project directory look correct?
CheckDirContains("inc", {"net/if.h", "tcp.h"}) -- TODO: Add all the important headers in a table and call in place of this inline variable
CheckDirContains("src", MakefileCoreSource())

-- Create a basic C file to test the compiler
Compiler = {}
CheckAssembler()
CheckCompiler()

-- Check size of standard types an actual 32-bit typedef can be defined
CheckCompilerNative() -- Can these tests even be done?
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
