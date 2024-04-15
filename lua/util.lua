function UniqueName()
	local filename = tostring(os.time()):sub(-8)
	local file = io.open(filename, "r")

	while file do
		file:close()
		filename = tostring(tonumber(filename) + 1):sub(-8)
		file = io.open(filename, "r")
	end

	return filename
end

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

function CreateCTestFile(name, src)
	if not src then src = "int main(void) {\n\treturn 0;\n}\n" end

	local file = io.open(name, "w")
	if file then
		file:write(src)
		file:close()
		return true
	end

	return false
end

function FileExists(name)
	local file = io.open(name)
	if file then
		file:close()
		return true
	end

	return false
end

function SanitizePath(path)
	local sanitize
	if System.family == "Unix" then
		sanitize = path:gsub([[\]], [[/]])
	else
		sanitize = path:gsub([[/]], [[\]])
	end

	return sanitize
end

function SearchForExecutable(exec, delimiter)
	local path = os.getenv("PATH")

	for filePath in path:gmatch("[^" .. delimiter .. "]+") do
		local path = filePath .. "/" .. exec

		local file = io.open(path)

		if file then
			file:close()
			return true
		end
	end

	return false
end

function RunCommandLocal(exec)
	if System.family == "Unix" then exec = "./" .. exec end
	RunCommand(exec)
end

function RunCommand(exec)
	os.execute(exec)
	-- Keep memory free on DOS systems
	collectgarbage("step", 9001)
end

function Check(msg)
	io.write(msg .. "... ")
	io.flush()
end

function Pass(msg)
	print(msg)
end

function Fail(msg)
	print(msg)
	os.exit(1)
end

function Error()
	print("Error!")
	os.exit(2)
end
