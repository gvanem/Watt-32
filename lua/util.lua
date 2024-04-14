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

function CreateCTestFile(name, src)
	if not src then
		src = "int main(void) {\n\treturn 0;\n}\n"
	end

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
	return RunCommand(exec)
end

function RunCommand(exec)
	local handle = {os.execute(exec)}

	if handle[1] == true and handle[2] == "exit" then
		return handle[3]
	end

	return nil
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
