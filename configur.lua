require("lua.args")
require("lua.checks")
require("lua.makefile")
require("lua.util")

Target = {}
GetOpt()

-- Check parameters and which makefile to generate

Target.makefile = CheckMakefileRequestValid()

-- Can the system achieve basic things?
System = CheckSystemFamily()
TmpFolder = UniqueName()

CheckCreateDirCmd(TmpFolder)
CheckRemoveFileCmd(TmpFolder)
CheckRemoveDirCmd(TmpFolder)

-- Does the project directory look correct?
CheckDirContains("inc", {"net/if.h", "tcp.h"}) -- TODO: Add all the important headers in a table and call in place of this inline variable
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
