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
System.md = CheckCreateDirCmd(System.family, TmpFolder)
System.rm = CheckRemoveFileCmd(System.family, TmpFolder)
System.rd = CheckRemoveDirCmd(System.family, TmpFolder)

-- Does the project directory look correct?
CheckDirContains(System.family, "inc", {"net/if.h", "tcp.h"}) -- TODO: Add all the important headers in a table and call in place of this inline variable
CheckDirContains(System.family, "src", MakefileCoreSource())

-- Create a basic C file to test the compiler
CheckCompiler(System.family, Target.makefile)

-- Check size of standard types an actual 32-bit typedef can be defined
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
