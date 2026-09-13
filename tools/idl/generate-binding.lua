-- generate-binding.lua
-- Unified code generation entry point.
-- Usage: lua generate-binding.lua <idl_path> <template_path> <binding> <output_path> <prefix> [indent]
--   binding: module suffix for "bindings-<binding>" (e.g. "c" → bindings-c, "zig" → bindings-zig,
--            "flecs" → bindings-flecs reflection registration)
--   prefix : naming prefix string (e.g. "cgpu")
--   indent : optional, defaults to "\t"

local idl_path      = arg[1]
local template_path = arg[2]
local binding       = arg[3]
local output_path   = arg[4]
local prefix        = arg[5]
local indent        = arg[6]
local api_macro     = arg[7]

if not idl_path or not binding or not output_path then
	error("Usage: lua generate-binding.lua <idl_path> <template_path> <binding> <output_path> <prefix> [indent]")
end

local codegen = require "codegen"
local idl = codegen.idl(idl_path, prefix)

if api_macro then
    codegen._naming.api_macro = api_macro
end

local modname = "bindings-" .. binding
local gen = require(modname)

print ("Generating: ", output_path, "from", template_path)

local codes = gen(idl, template_path, output_path, indent or "\t", codegen._naming, idl_path)

local function to_lf(text)
	return (text:gsub("\r\n", "\n"))
end

local function changed(codes, outputfile)
	local out = io.open(outputfile, "rb")
	if out then
		local origin = out:read "a"
		out:close()
		return to_lf(origin) ~= to_lf(codes)
	end
	return true
end

if not changed(codes, output_path) then
	print("No change")
else
	local out = assert(io.open(output_path, "wb"))
	out:write((to_lf(codes):gsub("\n", "\r\n")))
	out:close()
	print("Output: " .. output_path)
end
