require("lua.util")

function CreateMasmTestFile(name, src)
	if not src then
		src =
[[
.model small
.stack 100h

.data
	helloMsg db 'Hello, MASM!', '$'

.code
main:
	mov ax, @data
	mov ds, ax

	mov ah, 09h         ; Function to print string
	lea dx, helloMsg    ; Load address of the message
	int 21h             ; Call DOS interrupt to print the string

	mov ah, 4Ch         ; DOS function to terminate program
	int 21h             ; Call DOS interrupt

end main
]]
	end

	local file = io.open(name, "w")
	if file then
		file:write(src)
		file:close()
		src = nil
		return true
	end

	return false
end

function CreateGasTestFile(name, src)
	if not src then
		src =
[[
.section .data
hello_msg:
	.ascii "Hello, GAS!\0"
.section .text
.global _start
_start:
	mov $0x2, %ax
	mov %ax, %ds

	mov $0x09, %ah      # Function to print string
	mov $hello_msg, %dx # Load address of the message
	int $0x21           # Call DOS interrupt to print the string

	mov $0x4c, %ah      # DOS function to terminate program
	int $0x21           # Call DOS interrupt
]]
	end

	local file = io.open(name, "w")
	if file then
		file:write(src)
		file:close()
		src = nil
		return true
	end

	return false
end

function CheckCustomAssembler(as, tmpName)
	Check("Checking '" .. as .. "' compiler understands MASM format")

	if Target.skipChecks then
		Pass("Skipped")
		return
	end

	if not CreateMasmTestFile(tmpName .. ".asm") then Error() end
	RunCommand(as .. " " .. tmpName .. ".asm")
	os.remove(tmpName .. ".asm")

	local exists = CheckAndRemoveCommonArtifacts(tmpName)
	if exists > 0 then
		Pass("Yes")
		Compiler.as = as
		Compiler.aext = ".asm"
	else
		Pass("No")
		Check("Checking '" .. as .. "' compiler understands GAS format")

		if not CreateGasTestFile(tmpName .. ".s") then Error() end
		RunCommand(as .. " " .. tmpName .. ".s")
		os.remove(tmpName .. ".s")
		exists = CheckAndRemoveCommonArtifacts(tmpName)
		if exists > 0 then
			Pass("Yes")
			Compiler.as = as
			Compiler.aext = ".s"
			Pass("Yes")
		end
	end

	local name

	if Compiler.aext == "masm" then
		if not CreateMasmTestFile(tmpName .. ".asm") then Error() else
			name = tmpName .. ".asm"
		end
	else
		if not CreateGasTestFile(tmpName .. ".s") then Error() else
			name = tmpName .. ".s"
		end
	end

	local aflags = CheckEnvVar("AFLAGS")
	if aflags then
		Check("Checking if C compiler works with CFLAGS")
		local r = RunCommand (as .. " " .. aflags .. " " .. name)

		local exists = CheckAndRemoveCommonArtifacts(tmpName)
		if exists > 0 then Pass("Yes")
		else
			os.remove(name)
			Fail("No")
		end

		Compiler.aflags = aflags
	end

	os.remove(name)
end

function CheckGccAssembler(as, tmpName)
	local gcc = as or "as"

	Check("Checking '" .. gcc .. "' is available")

	if Target.skipChecks then
		Pass("Skipped")
		Compiler.as = gcc
		Compiler.aext = ".s"
		return
	end

	if not CreateGasTestFile(tmpName .. ".s") then Error() end

	RunCommand (gcc .. " " .. tmpName .. ".s ")

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes")
	else
		os.remove(tmpName .. ".s")
		Fail("No")
	end

	os.remove(tmpName .. ".s")
	Compiler.as = gcc
	Compiler.aext = ".s"
end

function CheckWasmAssembler(as, tmpName)
	local wasm = as or "wasm"

	Check("Checking " .. wasm .. " is available")

	if Target.skipChecks then
		Compiler.as = "wasm"
		Compiler.aext = ".asm"
		Pass("Skipped")
		return
	end

	if not CreateMasmTestFile(tmpName .. ".asm") then Error() end

	RunCommand (
		wasm .. " -bt=dos " ..
		tmpName .. ".asm"
	)

	os.remove(tmpName .. ".asm")

	local exist = CheckAndRemoveCommonArtifacts(tmpName)
	if exist > 0 then Pass("Yes") else Fail("No") end

	Compiler.as = wasm
	Compiler.aext = ".asm"
end
