require("lua.checks")
require("lua.util")

-- Get which makefile to generate
target = {}
target.makefile = CheckMakefileRequestValid(arg[1])

-- Can the system acheve basic things?
system = {}
system.family = CheckSystemFamily()
TmpFolder = UniqueName()

system.divider = system.family == "unix" and "/" or "\\"
system.md = CheckCreateDirCmd(system.family, TmpFolder)
system.rm = CheckRemoveFileCmd(system.family, TmpFolder)
system.rd = CheckRemoveDirCmd(system.family, TmpFolder)

-- Does the project directory look correct?
CheckDirContains(system.family, "inc", {"net/if.h", "tcp.h"})
CheckDirContains(system.family, "src", {"accept.c", "pcpkt.c"})

-- Create a basic C file to test the compiler
CheckCompiler(system.family, target.makefile)

Check("Checking if Lua configuration script is finished and ready to use")
Fail("No")

os.exit()
