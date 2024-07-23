--[[
	util.lua contains useful functions that have nowhere better to live.
]]

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

function GetExecutableNames(name)
	return {name, "a"}
end

function GetExecutableExtensions()
	return {".com", ".exe", ".out"}
end

function GetCompilerOutputExtensions()
	return {".a", ".com", ".dll", ".exe", ".lib", ".o", ".obj", ".out"}
end

function CheckAndReturnCommonExecutable(baseName)
	local names = GetExecutableNames(baseName)
	local extensions = GetExecutableExtensions()

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				file:close()
				return fileName
			end
		end
	end
end

function CheckAndRemoveCommonArtifacts(baseName)
	local names = GetExecutableNames(baseName)
	local extensions = GetCompilerOutputExtensions()
	local r = 0

	for _, name in ipairs(names) do
		for _, extension in ipairs(extensions) do
			local fileName = name .. extension
			local file = io.open(fileName)

			if file then
				file:close()
				os.remove(fileName)
				r = r + 1
			end
		end
	end

	return r
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

function StringToHexArray(str)
	local hexArray = {}

	for i = 1, #str do
		hexArray[#hexArray + 1] = string.format("0x%02X", string.byte(str, i))
	end

	return "[" .. table.concat(hexArray, ", ") .. "]"
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

function mkdir(path)
	Check("Creating folder '" .. path .. "'")
	local currentPath = "."
	for part in string.gmatch(path, "[^" .. System.divider .. "]+") do
		currentPath = SanitizePath(currentPath .. System.divider .. part)
		if not FileExists(currentPath) then
			RunCommand(System.md .. ' ' .. currentPath)
			if not FileExists(currentPath) then
				Error()
			end
		end
	end

	if FileExists(path) then
		Pass("Done")
	else
		Error()
	end
end
