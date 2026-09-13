local codegen = require "codegen"

local ENTITY_TYPE = "ecs_entity_t"

local function normalize_path(p)
	return (tostring(p):gsub("\\", "/"))
end

local function source_display(idl_path)
	return (normalize_path(idl_path):gsub("^%.%./", ""):gsub("^%.%./", ""))
end

local function module_of(idl_path)
	local module = normalize_path(idl_path):match "([^/]+)%.idl$"
	if not module or not module:match "^pulse_" then
		error("reflection: 无法从 " .. tostring(idl_path) .. " 推导模块名（需要 pulse_<name>.idl）")
	end
	return module
end

local function double_literal(value)
	if type(value) == "string" then
		return value
	end
	if type(value) ~= "number" then
		error("reflection: min/max 需要数字，当前为 " .. type(value))
	end
	local text = tostring(value)
	if not text:find("[%.eE]") then
		text = text .. ".0"
	end
	return text
end

local function pointer_like(item)
	if item.optional or item.slice or item.ptr then
		return true
	end
	return (item.ctype or ""):match "%*$" ~= nil
end

local function is_entity(item)
	return item.entity or item.type == ENTITY_TYPE or item.ctype == ENTITY_TYPE
end

local function array_count(item, owner)
	if not item.array then
		return 0
	end
	local at = item.array_at
	if not at or not at.number then
		error("reflection: " .. owner .. "." .. item.name .. " 的数组尺寸必须是数字字面量")
	end
	return tonumber(at.number)
end

local function range_suffix(item, owner)
	if item.min == nil and item.max == nil then
		return ""
	end
	if item.min == nil or item.max == nil then
		error("reflection: " .. owner .. "." .. item.name .. " 需要同时提供 min 和 max")
	end
	return string.format(".range(%s, %s)", double_literal(item.min), double_literal(item.max))
end

local function gen_member(cname, item)
	local owner = cname .. "." .. item.name
	if item.noreflex then
		if item.min ~= nil or item.max ~= nil then
			error("reflection: " .. owner .. " 标注了 noreflex，不能再设置 min/max")
		end
		return nil
	end
	if pointer_like(item) then
		if item.entity then
			error("reflection: " .. owner .. " 是指针类型，无法标注 entity")
		end
		return nil
	end
	local suffix = range_suffix(item, owner)
	if is_entity(item) then
		return string.format(
			"comp.member(ecs_id(%s), \"%s\", %d, offsetof(%s, %s))%s;",
			ENTITY_TYPE, item.cname, array_count(item, owner), cname, item.cname, suffix)
	end
	return string.format("comp.member(\"%s\", &%s::%s)%s;", item.cname, cname, item.cname, suffix)
end

local function gen_type(t)
	local cname = t.cname
	local lines = {}
	lines[#lines + 1] = "\t{"
	lines[#lines + 1] = string.format("\t\tflecs::component<%s> comp(world, \"%s\");", cname, cname)
	if t.tag then
		assert(#t.struct == 0, "tag can not have fields")
		lines[#lines + 1] = string.format("\t\tecs_id(%sId) = %sId = comp.id();", cname, cname)
	elseif not t.external then
		lines[#lines + 1] = string.format("\t\tecs_id(%s) = comp.id();", cname)
	end
	if not t.noreflex then
		for _, item in ipairs(t.struct) do
			local line = gen_member(cname, item)
			if line then
				lines[#lines + 1] = "\t\t" .. line
			end
		end
	end
	lines[#lines + 1] = "\t}"
	return lines
end

local function gen_reflection(idl)
	local lines = {}
	for _, t in ipairs(idl.types) do
		if t.component or t.tag then
			for _, line in ipairs(gen_type(t)) do
				lines[#lines + 1] = line
			end
		end
	end
	if #lines == 0 then
		error("reflection: idl 中没有 component/tag 定义，无需生成反射代码")
	end
	return table.concat(lines, "\n")
end

local function gen(idl, tempfile, outputfile, indent, naming, idl_path)
	local module = module_of(idl_path)
	local values = {
		source = source_display(idl_path),
		header = module .. ".h",
		function_name = module .. "_register_reflection",
		reflection = gen_reflection(idl),
	}
	return codegen.change_indent(codegen.apply_template(tempfile, values), indent)
end

return gen
