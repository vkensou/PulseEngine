-- generate.lua
-- Generate the `pulse_package_register` implementation for a pulse module
-- from its IDL plugin descriptor (e.g. PulseWindowPluginDesc).
--
-- The generator reuses the IDL toolchain in ../idl (codegen.lua / idl.lua).
--
-- Usage:
--   lua54 generate.lua <module_idl_path>
--
-- Output: <module>/src/package_register.cpp
--
-- Example:
--   lua54 generate.lua ..\..\src\pulse_window\idl\pulse_window.idl
--     -> ..\..\src\pulse_window\src\package_register.cpp
--
-- Batch processing is driven externally (see generate.bat): invoke this
-- script once per module idl.
--
-- Modes:
--   * with plugin desc (WindowPluginDesc / AssetPluginDesc / ...):
--       build the desc from PulseConfig (get_bool/get_int/get_double/get_string;
--       flags through get_int; nested value structs through get_obj),
--       then call pulse_add_xxx_plugin(app, &desc).
--   * without plugin desc (InputPluginDescVersion only, e.g. pulse_input):
--       config must be NULL, call pulse_add_xxx_plugin(app).
--
-- Config mapping rules (per desc field):
--   * struct_size / version   -> skipped (struct metadata)
--   * pointer / array / optional fields -> skipped
--   * funcptr / unknown external types  -> skipped
--   * bool                    -> pulse_config_get_bool
--   * float / double          -> pulse_config_get_double
--   * int types               -> pulse_config_get_int (+ cast)
--   * cstring                 -> pulse_config_get_string
--   * flag / enum types       -> (EPulseX)(uint32_t)pulse_config_get_int
--   * nested value struct     -> pulse_config_get_obj, then recurse

local function normalize_path(p)
    return (p:gsub("\\", "/"))
end

-- lexically resolve "." and ".." segments (path must be absolute, / separators)
local function collapse_dots(p)
    local root = p:match("^[A-Za-z]:") and "" or "/"
    local parts = {}
    for seg in p:gmatch("[^/]+") do
        if seg == "." then
            -- skip
        elseif seg == ".." then
            table.remove(parts)
        else
            parts[#parts + 1] = seg
        end
    end
    return root .. table.concat(parts, "/")
end

local function abspath(p)
    if p:match("^[A-Za-z]:") or p:match("^/") then
        return collapse_dots(normalize_path(p))
    end
    local cwd = io.popen("cd"):read("*l") or "."
    return collapse_dots(normalize_path(cwd) .. "/" .. normalize_path(p))
end

local script_path = abspath(arg[0] or "generate.lua")
local script_dir  = script_path:match("^(.*)/[^/]+$")
local repo_root   = abspath(script_dir .. "/../..")

package.path = script_dir .. "/../idl/?.lua;" .. script_dir .. "/?.lua;" .. package.path

local codegen = require "codegen"

local INT_TYPES = {
    int8_t = true, int16_t = true, int32_t = true, int64_t = true,
    uint8_t = true, uint16_t = true, uint32_t = true, uint64_t = true,
    uintptr_t = true, size_t = true,
}

local function find_func(funcs, name)
    for _, f in ipairs(funcs) do
        if f.name == name and not f.class then
            return f
        end
    end
    return nil
end

local function find_add_func(funcs)
    for _, f in ipairs(funcs) do
        if f.name:match("^Add%w+Plugin$") and not f.class then
            return f
        end
    end
    return nil
end

local function camel_to_underscore(name)
    local tmp = {}
    for v in name:gmatch "[%u%d]+[%l%d]*" do
        tmp[#tmp + 1] = v:lower()
    end
    return table.concat(tmp, "_")
end

-- Extract the item names of `enum.<enum_name> { ... }` from raw idl text.
-- The header attributes live in `{ }`; the items follow on later lines and
-- end with a line containing "()".
-- (Text-level parse, because loading two idl files into one codegen state
-- would collide on the shared primitive typedefs like "bool".)
local function idl_enum_items(idl_text, enum_name)
    local items = {}
    local in_enum = false
    for line in idl_text:gmatch("[^\r\n]+") do
        if not in_enum then
            if line:match("^%s*enum%." .. enum_name .. "%s*{") then
                in_enum = true
            end
        else
            if line:match("%(%s*%)%s*$") then
                return items
            end
            local n = line:match("^%s*%.([%w]+)")
            if n then
                items[#items + 1] = n
            end
        end
    end
    return nil
end

local function enum_item_cname(enum_base, item, prefix)
    local L_ = prefix:lower() .. "_"
    local item_snake = camel_to_underscore(item)
    return (L_ .. camel_to_underscore(enum_base)):upper() .. "_" .. item_snake:upper()
end

-- Map every EPulseAppAddPluginResult item to an EPulseResult item with the
-- same suffix; items without a counterpart fall back to ERROR_INTERNAL.
local function gen_result_switch(app_idl_text)
    local prefix = "Pulse"
    local add_items = idl_enum_items(app_idl_text, "AppAddPluginResult")
    local result_items = idl_enum_items(app_idl_text, "Result")
    if add_items and result_items then
        local result_map = {}
        for _, item in ipairs(result_items) do
            result_map[item:lower()] = enum_item_cname("Result", item, prefix)
        end
        local cases = {}
        for _, item in ipairs(add_items) do
            local ret = result_map[item:lower()] or "PULSE_RESULT_ERROR_INTERNAL"
            cases[#cases + 1] = string.format(
                "        case %s: return %s;",
                enum_item_cname("AppAddPluginResult", item, prefix), ret)
        end
        return cases
    end

    -- pulse_app.idl not parseable: fall back to the canonical mapping
    local base = "PULSE_APP_ADD_PLUGIN_RESULT_"
    local ret_base = "PULSE_RESULT_"
    local fallback = {
        { "OK", "OK" },
        { "ERROR_INVALID_ARGUMENT", "ERROR_INVALID_ARGUMENT" },
        { "ERROR_INVALID_STATE", "ERROR_INVALID_STATE" },
        { "ERROR_DUPLICATE_PLUGIN", "ERROR_DUPLICATE_PLUGIN" },
    }
    local cases = {}
    for _, pair in ipairs(fallback) do
        cases[#cases + 1] = string.format(
            "        case %s%s: return %s%s;",
            base, pair[1], ret_base, pair[2])
    end
    return cases
end

-- Generate 'desc.<member> = pulse_config_get_xxx(...)' lines for one struct.
-- desc_ref: C expression of the struct lvalue (e.g. "desc" or "desc.primary_window")
-- struct:   IDL struct model
-- cfg_var:  PulseConfig* variable holding the config node
-- types:    idl.types (name-indexed type table)
local function gen_mapping_lines(desc_ref, struct, cfg_var, indent, visited, types)
    local lines = {}
    for _, item in ipairs(struct.struct) do
        if item.cname == "struct_size" or item.cname == "version" then
            -- struct metadata, always filled by the default function
        elseif item.ptr or item.array or item.optional then
            -- pointer / array fields cannot be mapped from scalar config
        else
            local ft = item.fulltype
            local target = desc_ref .. "." .. item.cname
            local key = item.cname
            if ft == "bool" then
                lines[#lines + 1] = string.format(
                    "%s%s = pulse_config_get_bool(%s, \"%s\", %s);",
                    indent, target, cfg_var, key, target)
            elseif ft == "cstring" then
                lines[#lines + 1] = string.format(
                    "%s%s = pulse_config_get_string(%s, \"%s\", %s);",
                    indent, target, cfg_var, key, target)
            elseif ft == "double" then
                lines[#lines + 1] = string.format(
                    "%s%s = pulse_config_get_double(%s, \"%s\", %s);",
                    indent, target, cfg_var, key, target)
            elseif ft == "float" then
                lines[#lines + 1] = string.format(
                    "%s%s = (float)pulse_config_get_double(%s, \"%s\", (double)%s);",
                    indent, target, cfg_var, key, target)
            elseif INT_TYPES[ft] then
                if ft == "int64_t" then
                    lines[#lines + 1] = string.format(
                        "%s%s = pulse_config_get_int(%s, \"%s\", %s);",
                        indent, target, cfg_var, key, target)
                else
                    lines[#lines + 1] = string.format(
                        "%s%s = (%s)pulse_config_get_int(%s, \"%s\", %s);",
                        indent, target, ft, cfg_var, key, target)
                end
            else
                local t = types[ft]
                if t and (t.flag or t.enum) then
                    lines[#lines + 1] = string.format(
                        "%s%s = (%s)(uint32_t)pulse_config_get_int(%s, \"%s\", (int64_t)%s);",
                        indent, target, t.cname, cfg_var, key, target)
                elseif t and t.struct and not visited[t.name] then
                    -- nested value struct: read a sub-config object
                    visited[t.name] = true
                    local sub = cfg_var .. "_" .. item.cname
                    lines[#lines + 1] = string.format(
                        "%sPulseConfig* %s = pulse_config_get_obj(%s, \"%s\");",
                        indent, sub, cfg_var, key)
                    lines[#lines + 1] = string.format("%sif (%s) {", indent, sub)
                    for _, l in ipairs(gen_mapping_lines(
                        target, t, sub, indent .. "    ", visited, types)) do
                        lines[#lines + 1] = l
                    end
                    lines[#lines + 1] = string.format("%s}", indent)
                end
                -- else: unknown external type -> skip
            end
        end
    end
    return lines
end

local function write_output(out_path, text)
    text = text:gsub("\r\n", "\n")
    local changed = true
    local f = io.open(out_path, "rb")
    if f then
        local old = f:read("a")
        f:close()
        changed = old:gsub("\r\n", "\n") ~= text
    end
    if not changed then
        print("No change: " .. out_path)
        return
    end
    local out = assert(io.open(out_path, "wb"), "cannot open " .. out_path)
    -- note: parenthesize gsub - it returns (string, count) and the count
    -- would otherwise be written to the file as an extra argument
    out:write((text:gsub("\n", "\r\n")))
    out:close()
    print("Output: " .. out_path)
end

local function generate_one(idl_path)
    idl_path = abspath(idl_path)
    local module_dir = idl_path:gsub("idl/[^/]+$", ""):gsub("/$", "")
    local modname = module_dir:match("([^/]+)$")
    if not modname or not modname:match("^pulse_") then
        error("not a pulse module idl: " .. idl_path)
    end

    local api_macro = modname:upper() .. "_API"
    local header = modname .. ".h"
    local out_path = module_dir .. "/src/package_register.cpp"

    print("Generating: " .. out_path .. " from " .. idl_path)

    -- pulse_app.idl provides the EPulseResult / EPulseAppAddPluginResult
    -- item names, read at text level to avoid type collisions in codegen.
    local app_idl_file = io.open(repo_root .. "/src/pulse_app/idl/pulse_app.idl", "rb")
    local app_idl_text
    if app_idl_file then
        app_idl_text = app_idl_file:read("a")
        app_idl_file:close()
    end
    app_idl_text = app_idl_text or ""

    local idl = codegen.idl(idl_path, "Pulse")

    -- find the plugin descriptor struct: struct.XxxPluginDesc
    local plugin_desc
    local desc_base
    for _, t in ipairs(idl.types) do
        if t.struct then
            local b = t.name:match("^(%w+)PluginDesc$")
            if b then
                plugin_desc, desc_base = t, b
                break
            end
        end
    end

    -- find the add-plugin function
    local add_func
    local default_func
    local use_desc = false
    if plugin_desc and desc_base then
        add_func = find_func(idl.funcs, "Add" .. desc_base .. "Plugin")
        default_func = find_func(idl.funcs, desc_base .. "PluginDescDefault")
        if add_func and default_func then
            use_desc = true
        end
    end
    if not add_func then
        add_func = find_add_func(idl.funcs)
    end
    if not add_func then
        print("  skip: no plugin Add function in " .. idl_path)
        return
    end

    local L_ = codegen._naming.L_

    print("  plugin desc : " .. (plugin_desc and plugin_desc.name or "(none)"))
    print("  add function: " .. L_ .. add_func.cname)

    local body = {}

    -- function signature
    body[#body + 1] = string.format(
        "    %s EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config) {",
        api_macro)

    if use_desc then
        body[#body + 1] = string.format("    %s desc = %s();",
            plugin_desc.cname, L_ .. default_func.cname)
    end

    -- config mapping
    if use_desc then
        local mapping = gen_mapping_lines("desc", plugin_desc, "config", "        ", {}, idl.types)
        if #mapping > 0 then
            body[#body + 1] = "    if (config) {"
            for _, l in ipairs(mapping) do
                body[#body + 1] = l
            end
            body[#body + 1] = "    }"
        end
    else
        body[#body + 1] = "    if (config) {"
        body[#body + 1] = "        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;"
        body[#body + 1] = "    }"
    end

    -- call + result mapping
    if use_desc then
        body[#body + 1] = string.format("    EPulseAppAddPluginResult r = %s(app, &desc);", L_ .. add_func.cname)
    else
        body[#body + 1] = string.format("    EPulseAppAddPluginResult r = %s(app);", L_ .. add_func.cname)
    end
    body[#body + 1] = "    switch (r) {"
    for _, c in ipairs(gen_result_switch(app_idl_text)) do
        body[#body + 1] = c
    end
    body[#body + 1] = "        default: return PULSE_RESULT_ERROR_INTERNAL;"
    body[#body + 1] = "    }"
    body[#body + 1] = "}"

    local header_comment = "// ============================================================================\n// package_register.cpp\n// GENERATED FILE - DO NOT EDIT.\n//\n// Generated by tools/gen_package_register/generate.lua\n// Source: " ..
        idl_path:gsub("^" .. (repo_root:gsub("%p", "%%%1")) .. "/", "") ..
        "\n// ============================================================================\n"

    local text = header_comment .. [[
#include "pulse_config.h"
#include "]] .. header .. [["

extern "C" {

]] .. table.concat(body, "\n") .. [[

} // extern "C"
]]

    write_output(out_path, text)
end

-- ============================================================================

local idl_arg = arg[1]
if not idl_arg then
    error("Usage: lua generate.lua <module_idl_path>")
end

generate_one(idl_arg)