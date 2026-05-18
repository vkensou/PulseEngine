--[[
  用法: lua generate_module.lua <dump|generate> [头文件名.h]
  默认头文件: snake.h
  dump: 仅打印解析结果。 generate: 生成 <文件名>_module.{h,cpp} 文件。
  旧版用法: lua generate_module.lua foo.h 等同于 generate foo.h
]]
local function collapse_ws(s)
    return (s:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", ""))
end

-- 宏解析阶段（与 PULSE_ECS_SYSTEM / init、update、imgui 管线对应）
local PHASE = {
    INIT = "INIT",
    UPDATE = "UPDATE",
    IMGUI = "IMGUI",
}

-- 参数种类（与 ECS Module 规则一致）
local KIND = {
    COMMAND_BUFFER = "command_buffer",
    EVENT_READER = "event_reader",
    EVENT_WRITER = "event_writer",
    SINGLETON_QUERY = "singleton_query",
    RES = "res",
    FLECS_QUERY = "flecs_query",
    FLECS_ENTITY = "flecs_entity",
    COMPONENT = "component",
}

local PIPELINE_FIELD = {
    [PHASE.INIT] = "initPipeline",
    [PHASE.UPDATE] = "updatePipeline",
    [PHASE.IMGUI] = "imguiPipeline",
}

local PHASE_ORDER = { PHASE.INIT, PHASE.UPDATE, PHASE.IMGUI }

local function read_file(filename)
    local f = io.open(filename, "r")
    if not f then error("无法打开文件: " .. filename) end
    local content = f:read("*a")
    f:close()
    return content
end

local function write_file(filename, content)
    local f = io.open(filename, "w")
    if not f then error("无法写入文件: " .. filename) end
    f:write(content)
    f:close()
end

local function parse_phase_from_macro_block(block)
    if block:find("PHASE=" .. PHASE.INIT, 1, true) then
        return PHASE.INIT
    elseif block:find("PHASE=" .. PHASE.UPDATE, 1, true) then
        return PHASE.UPDATE
    elseif block:find("PHASE=" .. PHASE.IMGUI, 1, true) then
        return PHASE.IMGUI
    end
    return nil
end

-- 解析 `PULSE_ECS_SYSTEM(...)` 括号内是否包含独立项 `IMMEDIATE`（与 `PHASE=...` 等逗号分隔）。
local function parse_immediate_from_macro_block(block)
    local inner = block:match("PULSE_ECS_SYSTEM%(([^)]*)%)")
    if not inner then
        return false
    end
    for piece in inner:gmatch("[^,]+") do
        local token = piece:match("^%s*(.-)%s*$")
        if token == "IMMEDIATE" then
            return true
        end
    end
    return false
end

-- 规范化参数类型，供 classify_param_type 使用 (移除引用和前置 const)。
local function normalize_param_type_for_kind(typ)
    typ = collapse_ws(typ)
    typ = typ:gsub("%s*&%s*$", "")
    typ = typ:gsub("^const%s+", "")
    return typ
end

-- 分类规则顺序敏感（先匹配 command_buffer / event_* / singleton / res / query / entity）。
local TYPE_KIND_RULES = {
    {
        test = function(t, norm)
            return norm == "pulse::command_buffer" or t:find("^pulse::command_buffer") ~= nil
        end,
        kind = KIND.COMMAND_BUFFER,
    },
    {
        test = function(t, norm)
            return t:find("^pulse::event_reader%s*<") ~= nil or norm:find("^pulse::event_reader") ~= nil
        end,
        kind = KIND.EVENT_READER,
    },
    {
        test = function(t, norm)
            return t:find("^pulse::event_writer%s*<") ~= nil or norm:find("^pulse::event_writer") ~= nil
        end,
        kind = KIND.EVENT_WRITER,
    },
    {
        test = function(t, norm)
            return t:find("^pulse::singleton_query%s*<") ~= nil or norm:find("^pulse::singleton_query") ~= nil
        end,
        kind = KIND.SINGLETON_QUERY,
    },
    {
        test = function(t, norm)
            return t:find("^pulse::res%s*<") ~= nil or norm:find("^pulse::res") ~= nil
        end,
        kind = KIND.RES,
    },
    {
        test = function(t, norm)
            return t:find("^flecs::query%s*<") ~= nil or norm:find("^flecs::query") ~= nil
        end,
        kind = KIND.FLECS_QUERY,
    },
    {
        test = function(_, norm)
            return norm == "flecs::entity"
        end,
        kind = KIND.FLECS_ENTITY,
    },
}

local function classify_param_type(raw_type)
    local t = collapse_ws(raw_type)
    local norm = normalize_param_type_for_kind(t)
    for _, rule in ipairs(TYPE_KIND_RULES) do
        if rule.test(t, norm) then
            return rule.kind
        end
    end
    return KIND.COMPONENT
end

local function type_for_main_query_list(raw_type)
    local t = collapse_ws(raw_type)
    t = t:gsub("%s*&%s*$", "")
    return t
end

local function build_main_query_components(params)
    local list = {}
    for _, p in ipairs(params) do
        local cat = classify_param_type(p.type)
        if cat == KIND.COMPONENT then
            table.insert(list, type_for_main_query_list(p.type))
        end
    end
    return list
end

local function is_entity_system(params)
    for _, p in ipairs(params) do
        local cat = classify_param_type(p.type)
        if cat == KIND.FLECS_ENTITY or cat == KIND.COMPONENT then
            return true
        end
    end
    return false
end

-- 提取顶层第一对 <...> 中的内容 (尖括号需配平)。
local function extract_template_type(param_type)
    local start = param_type:find("<", 1, true)
    if not start then
        return nil
    end
    local depth = 0
    for i = start, #param_type do
        local c = param_type:sub(i, i)
        if c == "<" then
            depth = depth + 1
        elseif c == ">" then
            depth = depth - 1
            if depth == 0 then
                return param_type:sub(start + 1, i - 1)
            end
        end
    end
    return nil
end

local function count_event_readers(params)
    local n = 0
    local ev = nil
    for _, p in ipairs(params) do
        if classify_param_type(p.type) == KIND.EVENT_READER then
            n = n + 1
            ev = extract_template_type(p.type)
        end
    end
    return n, ev
end

local WRAPPER_NAME_OVERRIDES = {
    ["onImguiSystem"] = "onImguiWrapper",
}

local function wrapper_function_name(system)
    return WRAPPER_NAME_OVERRIDES[system.name] or (system.name .. "Wrapper")
end

local function event_reader_param_name(system)
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.EVENT_READER then
            if p.name ~= "" then
                return p.name
            end
            break
        end
    end
    return "eventReader"
end

-- 事件 + 实体 system，且无附加 flecs::query、主查询仅一个组件时：在事件 Wrapper 内用主 query 的 `.each` 调用原函数。
local function event_entity_uses_inner_each(system)
    return system.has_event_reader and system.entity_system and #system.extra_queries == 0
        and #system.main_query_components == 1
end

local function first_component_param(system)
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.COMPONENT then
            return p
        end
    end
    return nil
end

local function normalize_type_for_output(typ)
    typ = typ:gsub("%s*&&", "&")
    typ = typ:gsub("^const%s+const%s+", "const ")
    typ = typ:gsub("const%s+const%s+", "const ")
    typ = typ:gsub("%s+", " ")
    typ = typ:gsub("^%s+", "")
    typ = typ:gsub("%s+$", "")
    return typ
end

local function normalize_type_for_registration(typ)
    if typ:find("flecs::query") then
        local inner = extract_template_type(typ)
        if inner then
            return inner
        end
    end
    typ = typ:gsub("%s*&", "")
    typ = typ:gsub("%s+", " ")
    typ = typ:gsub("^%s+", "")
    typ = typ:gsub("%s+$", "")
    return typ
end

-- 移除模板参数中的前置 `const` (用于 `get_mut<T>`)。
local function template_inner_strip_leading_const(inner)
    if not inner then
        return nil
    end
    return inner:gsub("^const%s+", "")
end

local function singleton_query_local_name(p)
    if p.name:match("Query$") then
        return p.name
    end
    return p.name .. "Query"
end

local function type_with_ref_suffix(typ)
    typ = normalize_type_for_output(typ)
    if typ:match("&%s*$") then
        return typ
    end
    return typ .. "&"
end

local function get_event_wrapper_signature_params(system)
    local sig = {}
    if event_entity_uses_inner_each(system) then
        local ct = system.main_query_components[1]
        local inner = ct:gsub("^const%s+", "")
        local cp = first_component_param(system)
        local qname = (cp and (cp.name .. "Query")) or "mainQuery"
        table.insert(sig, { kind = "synth_query", type = "flecs::query<" .. inner .. ">&", name = qname })
        return sig
    end
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.FLECS_QUERY then
            local ct = normalize_type_for_output(p.type)
            ct = ct:gsub("%s*&&", "&")
            table.insert(sig, { kind = "query", type = ct, name = p.name })
        end
    end
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.COMPONENT then
            table.insert(sig, { kind = "component", type = type_with_ref_suffix(p.type), name = p.name })
        end
    end
    return sig
end

local function system_has_command_buffer_param(system)
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.COMMAND_BUFFER then
            return true
        end
    end
    return false
end

-- 无事件 reader、且含 `flecs::query` 参数：附加 query 存 system state；管理器 system 用 `.run` + `set`，实体 system 用 `.each` + `set`（与 PHASE 无关）。
local function system_needs_attached_query_state(system)
    if system.has_event_reader then
        return false
    end
    if #system.extra_queries == 0 then
        return false
    end
    return true
end

-- 实体 system 走 `.each`（附加 query 仍来自 systemState）；管理器走 `.run`。与 INIT/UPDATE/IMGUI 无关。
local function attached_query_registration_uses_each(sys)
    return sys.entity_system
end

local function wrapper_state_struct_name(system)
    return wrapper_function_name(system) .. "State"
end

-- `flecs::query<...>` 成员类型（无引用）。
local function type_for_stored_query_member(param_type)
    local t = normalize_type_for_output(param_type)
    t = t:gsub("%s*&&", "")
    t = t:gsub("%s*&%s*$", "")
    return t
end

local function attached_query_field_name(query_entry, ordinal)
    if query_entry.name and query_entry.name ~= "" then
        return query_entry.name
    end
    return "attachedQuery" .. ordinal
end

local function generate_attached_query_state_struct(system)
    local state_name = wrapper_state_struct_name(system)
    local lines = {}
    table.insert(lines, "struct " .. state_name)
    table.insert(lines, "{")
    for i, q in ipairs(system.extra_queries) do
        local fname = attached_query_field_name(q, i)
        local mem_t = type_for_stored_query_member(q.type)
        table.insert(lines, string.format("\t%s %s;", mem_t, fname))
    end
    table.insert(lines, "};")
    return table.concat(lines, "\n")
end

-- 参数在 `system.params` 中的下标若为 `flecs::query`，返回其在所有 query 参数中的序号 (1..n)。
local function query_ordinal_for_param_at(system, param_idx)
    local ord = 0
    for j = 1, param_idx do
        if classify_param_type(system.params[j].type) == KIND.FLECS_QUERY then
            ord = ord + 1
        end
    end
    return ord
end

-- `pulse::res` / `pulse::res<const T>`：一次计算得到待插入行与调用实参。
local function res_emit_binding(p)
    local inner_raw = extract_template_type(p.type)
    if not inner_raw then
        error("`res` 类型缺少模板参数: " .. p.type)
    end
    local plain = template_inner_strip_leading_const(inner_raw)
    local is_const_res = p.type:find("pulse::res<const", 1, true) ~= nil
    local q = p.name .. "Query"
    local lines = {}
    if not is_const_res then
        table.insert(lines, string.format("\tauto& %s = world.get_mut<%s>();", q, plain))
    else
        table.insert(lines, string.format("\tauto& %s = world.get<%s>();", q, inner_raw))
    end
    local call_arg = string.format("pulse::res<%s>(%s)", inner_raw, q)
    return { lines = lines, call_arg = call_arg }
end

local function parse_param(param_str)
    param_str = collapse_ws(param_str)
    local depth = 0
    local last_space = nil
    for i = 1, #param_str do
        local c = param_str:sub(i, i)
        if c == "<" then depth = depth + 1
        elseif c == ">" then depth = depth - 1
        elseif c == " " and depth == 0 then
            last_space = i
        end
    end
    if last_space then
        local param_type = param_str:sub(1, last_space - 1):gsub("%s+$", "")
        local param_name = param_str:sub(last_space + 1):gsub("%s+$", "")
        return param_type, param_name
    end
    return param_str, ""
end

-- 从 `open_idx` 位置的 '(' 开始，查找匹配的 ')'；若不匹配则返回 nil。
local function match_closing_paren(content, open_idx, len)
    local depth = 0
    local pos = open_idx
    while pos <= len do
        local c = content:sub(pos, pos)
        if c == "(" then
            depth = depth + 1
        elseif c == ")" then
            depth = depth - 1
            if depth == 0 then
                return pos
            end
        end
        pos = pos + 1
    end
    return nil
end

local function skip_ws(content, pos, len)
    while pos <= len do
        local c = content:sub(pos, pos)
        if c ~= " " and c ~= "\t" and c ~= "\r" and c ~= "\n" then
            break
        end
        pos = pos + 1
    end
    return pos
end

local function parse_systems(content)
    local systems = {}
    local i = 1
    local len = #content

    while i <= len do
        local macro_start = content:find("PULSE_ECS_SYSTEM", i, true)
        if not macro_start then
            break
        end

        local macro_open_paren = content:find("%(", macro_start)
        if not macro_open_paren then
            i = macro_start + 1
        else
            local macro_close = match_closing_paren(content, macro_open_paren, len)
            if not macro_close then
                i = macro_start + 1
            else
                local macro_block = content:sub(macro_start, macro_close)
                local phase = parse_phase_from_macro_block(macro_block)
                if not phase then
                    i = macro_start + 1
                else
                    local scan = skip_ws(content, macro_close + 1, len)
                    local open_paren = content:find("%(", scan)
                    if not open_paren then
                        i = macro_close + 1
                    else
                        local fn_close = match_closing_paren(content, open_paren, len)
                        if not fn_close then
                            i = open_paren + 1
                        else
                        local params_str = content:sub(open_paren + 1, fn_close - 1)

                        local params = {}
                        if params_str and params_str ~= "" then
                            local param_idx = 1
                            local param_start = 1
                            local param_depth = 0

                            while param_idx <= #params_str do
                                local c = params_str:sub(param_idx, param_idx)
                                if c == "<" then
                                    param_depth = param_depth + 1
                                elseif c == ">" then
                                    param_depth = param_depth - 1
                                elseif c == "," and param_depth == 0 then
                                    local param = params_str:sub(param_start, param_idx - 1)
                                    local param_type, param_name = parse_param(param)
                                    if param_type and param_type ~= "" then
                                        table.insert(params, { type = param_type, name = param_name })
                                    end
                                    param_start = param_idx + 1
                                end
                                param_idx = param_idx + 1
                            end

                            local last_param = params_str:sub(param_start)
                            local param_type, param_name = parse_param(last_param)
                            if param_type and param_type ~= "" then
                                table.insert(params, { type = param_type, name = param_name })
                            end
                        end

                        local decl = content:sub(scan, open_paren - 1):gsub("%s+$", "")
                        local func_name = decl:match("([%w_]+)%s*$")
                        local ret_type = decl
                        if func_name then
                            ret_type = decl:sub(1, #decl - #func_name):gsub("%s+$", "")
                        end
                        if ret_type == "" then
                            ret_type = "void"
                        end

                        local sig_parts = {}
                        for _, p in ipairs(params) do
                            local piece = p.type
                            if p.name ~= "" then
                                piece = piece .. " " .. p.name
                            end
                            table.insert(sig_parts, piece)
                        end
                        local signature = string.format("%s %s(%s)", ret_type, func_name or "?", table.concat(sig_parts, ", "))

                        local event_reader_count, event_type = count_event_readers(params)
                        local has_event_reader = event_reader_count > 0
                        local entity_system = is_entity_system(params)
                        local main_query_components = build_main_query_components(params)
                        local extra_queries = {}
                        for _, p in ipairs(params) do
                            if classify_param_type(p.type) == KIND.FLECS_QUERY then
                                table.insert(extra_queries, { type = p.type, name = p.name })
                            end
                        end
                        local immediate = parse_immediate_from_macro_block(macro_block)

                        table.insert(systems, {
                            name = func_name,
                            ret_type = ret_type,
                            phase = phase,
                            immediate = immediate,
                            params = params,
                            signature = signature,
                            has_event_reader = has_event_reader,
                            event_reader_count = event_reader_count,
                            event_type = event_type,
                            entity_system = entity_system,
                            manager_system = not entity_system,
                            main_query_components = main_query_components,
                            extra_queries = extra_queries,
                            macro_block = macro_block,
                        })

                        i = fn_close + 1
                        end
                    end
                end
            end
        end
    end

    return systems
end

local function print_parsed_systems_dump(systems)
    print("========== ECS 解析结果 ==========")
    print(string.format("%d 个系统\n", #systems))
    for idx, sys in ipairs(systems) do
        print(string.rep("-", 72))
        print(string.format("[%d] %s", idx, sys.name))
        print("  宏: " .. sys.macro_block:gsub("\r?\n", " "))
        print(string.format("  阶段: %s", sys.phase))
        print(string.format("  IMMEDIATE: %s", sys.immediate and "是" or "否"))
        print(string.format("  返回类型: %s", sys.ret_type))
        print("  签名:")
        print("    " .. sys.signature)
        print(string.format("  种类: %s | %s",
            sys.has_event_reader and "事件" or "普通",
            sys.entity_system and "实体" or "管理器"))
        if sys.has_event_reader then
            print(string.format("  event_reader 数量: %d (应为 1)", sys.event_reader_count))
            if sys.event_type then
                print("  event_reader<T> 的 T 类型: " .. sys.event_type)
            end
        end
        local mq = sys.main_query_components
        if #mq > 0 then
            print("  主查询组件:")
            print("    " .. table.concat(mq, ", "))
        else
            print("  主查询: (无)")
        end
        if #sys.extra_queries > 0 then
            print("  额外 flecs::query:")
            for _, q in ipairs(sys.extra_queries) do
                print(string.format("    - %s  [%s]", q.type, q.name ~= "" and q.name or "<匿名>"))
            end
        end
        if system_needs_attached_query_state(sys) then
            local reg = attached_query_registration_uses_each(sys) and ".each" or ".run"
            print("  生成模式: system 附加 query state + " .. reg .. " + .set")
        end
        print("  参数:")
        for pi, p in ipairs(sys.params) do
            local cat = classify_param_type(p.type)
            print(string.format("    [%d] %-16s  %s  %s",
                pi, cat, p.type, p.name ~= "" and p.name or "<匿名>"))
        end
        print("")
    end
    print(string.rep("=", 72))
end

-- include_flecs_query=true：签名含附加 query，不含 `flecs::entity`；false：附加 query 走 systemState 时仅组件/entity。
local function should_include_param_in_wrapper_signature(p, include_flecs_query)
    local k = classify_param_type(p.type)
    if k == KIND.EVENT_READER or k == KIND.RES or k == KIND.COMMAND_BUFFER or
       k == KIND.SINGLETON_QUERY or k == KIND.EVENT_WRITER or p.name == "" then
        return false
    end
    if not include_flecs_query and k == KIND.FLECS_QUERY then
        return false
    end
    if include_flecs_query and k == KIND.FLECS_ENTITY then
        return false
    end
    return true
end

local function collect_wrapper_params(system, include_flecs_query)
    local out = {}
    for _, p in ipairs(system.params) do
        if should_include_param_in_wrapper_signature(p, include_flecs_query) then
            table.insert(out, { type = p.type, name = p.name })
        end
    end
    return out
end

local function event_writer_var_name(p)
    local event_t = extract_template_type(p.type)
    if not event_t then
        error("`event_writer` 缺少模板参数: " .. p.type)
    end
    if p.name ~= "" then
        return p.name
    end
    return event_t:gsub("Event$", "") .. "Writer"
end

local function emit_singleton_query_setup_line(output, p)
    local inner_type = extract_template_type(p.type)
    local sq = singleton_query_local_name(p)
    table.insert(output, string.format("\tauto %s = pulse::singleton_query<%s>(world);", sq, inner_type))
end

local function emit_event_writer_setup_line(output, p)
    local event_t = extract_template_type(p.type)
    local var_name = event_writer_var_name(p)
    table.insert(output, string.format("\tauto %s = pulse::event_writer<%s>(world);", var_name, event_t))
end

local function generate_wrapper(system)
    local output = {}

    if system.has_event_reader then
        return nil
    end

    if system_needs_attached_query_state(system) then
        local use_each_wrapper = system.entity_system
        if not use_each_wrapper then
            for _, p in ipairs(system.params) do
                if classify_param_type(p.type) == KIND.FLECS_ENTITY then
                    error(system.name .. ": flecs::entity 仅在实体 system（.each）时可用；当前为管理器路径。")
                end
            end
        end
        local state_name = wrapper_state_struct_name(system)
        table.insert(output, "")
        table.insert(output, "void " .. wrapper_function_name(system) .. "(")
        if not use_each_wrapper then
            table.insert(output, "\tflecs::iter& it)")
        else
            local wparams = collect_wrapper_params(system, false)
            table.insert(output, "\tflecs::iter& it, size_t i")
            for _, qp in ipairs(wparams) do
                table.insert(output, "\t, " .. type_with_ref_suffix(qp.type) .. " " .. qp.name)
            end
            table.insert(output, ")")
        end
        table.insert(output, "{")
        table.insert(output, "\tauto world = it.world();")
        local call_args = {}
        for pi, p in ipairs(system.params) do
            local k = classify_param_type(p.type)
            if k == KIND.RES then
                local rb = res_emit_binding(p)
                for _, ln in ipairs(rb.lines) do
                    table.insert(output, ln)
                end
                table.insert(call_args, rb.call_arg)
            elseif k == KIND.COMMAND_BUFFER then
                table.insert(output, "\tpulse::command_buffer command_buffer(world);")
                table.insert(call_args, "command_buffer")
            elseif k == KIND.SINGLETON_QUERY then
                emit_singleton_query_setup_line(output, p)
                table.insert(call_args, singleton_query_local_name(p))
            elseif k == KIND.EVENT_WRITER then
                emit_event_writer_setup_line(output, p)
                table.insert(call_args, event_writer_var_name(p))
            elseif use_each_wrapper and k == KIND.FLECS_ENTITY then
                table.insert(output, "\tauto entity = it.entity(i);")
                table.insert(call_args, "entity")
            elseif k == KIND.FLECS_QUERY then
                local ord = query_ordinal_for_param_at(system, pi)
                local qe = system.extra_queries[ord]
                table.insert(call_args, "systemState." .. attached_query_field_name(qe, ord))
            elseif p.name ~= "" then
                table.insert(call_args, p.name)
            end
        end
        table.insert(output, string.format("\tauto systemState = it.system().get<%s>();", state_name))
        table.insert(output, string.format("\t%s(%s);", system.name, table.concat(call_args, ", ")))
        table.insert(output, "}")
        return table.concat(output, "\n")
    end

    table.insert(output, "")
    table.insert(output, "void " .. wrapper_function_name(system) .. "(")

    local wrapper_params = collect_wrapper_params(system, true)

    if not system.entity_system then
        table.insert(output, "\tflecs::iter& it)")
    else
        if #wrapper_params > 0 then
            table.insert(output, "\tflecs::iter& it, size_t i")
            for _, qp in ipairs(wrapper_params) do
                table.insert(output, "\t, " .. type_with_ref_suffix(qp.type) .. " " .. qp.name)
            end
        else
            table.insert(output, "\tflecs::iter& it, size_t i")
        end
        table.insert(output, ")")
    end

    table.insert(output, "{")
    table.insert(output, "\tauto world = it.world();")

    local call_args = {}

    for _, p in ipairs(system.params) do
        local k = classify_param_type(p.type)
        if k == KIND.RES then
            local rb = res_emit_binding(p)
            for _, ln in ipairs(rb.lines) do
                table.insert(output, ln)
            end
            table.insert(call_args, rb.call_arg)
        elseif k == KIND.COMMAND_BUFFER then
            table.insert(output, "\tpulse::command_buffer command_buffer(world);")
            table.insert(call_args, "command_buffer")
        elseif k == KIND.SINGLETON_QUERY then
            emit_singleton_query_setup_line(output, p)
            table.insert(call_args, singleton_query_local_name(p))
        elseif k == KIND.EVENT_WRITER then
            emit_event_writer_setup_line(output, p)
            table.insert(call_args, event_writer_var_name(p))
        elseif k == KIND.FLECS_ENTITY then
            table.insert(output, "\tauto entity = it.entity(i);")
            table.insert(call_args, "entity")
        elseif k == KIND.FLECS_QUERY then
            table.insert(call_args, p.name)
        elseif p.name ~= "" then
            table.insert(call_args, p.name)
        end
    end

    local arg_str = table.concat(call_args, ", ")
    table.insert(output, string.format("\t%s(%s);", system.name, arg_str))
    table.insert(output, "}")

    return table.concat(output, "\n")
end

local function generate_event_wrapper(system)
    local output = {}

    if not system.has_event_reader then
        return nil
    end

    local er_name = event_reader_param_name(system)
    local wname = wrapper_function_name(system)
    local inner_each = event_entity_uses_inner_each(system)
    local sig_params = get_event_wrapper_signature_params(system)

    table.insert(output, "")
    table.insert(output, "void " .. wname .. "(")
    table.insert(output, string.format("\tpulse::event_reader<%s> %s, flecs::world& world", system.event_type, er_name))
    for _, sp in ipairs(sig_params) do
        table.insert(output, "\t, " .. sp.type .. " " .. sp.name)
    end
    table.insert(output, ")")
    table.insert(output, "{")

    local has_writer = false
    for _, p in ipairs(system.params) do
        if classify_param_type(p.type) == KIND.EVENT_WRITER then
            has_writer = true
            break
        end
    end

    local function emit_singletons_and_writers()
        for _, p in ipairs(system.params) do
            if classify_param_type(p.type) == KIND.SINGLETON_QUERY then
                emit_singleton_query_setup_line(output, p)
            end
        end
        for _, p in ipairs(system.params) do
            if classify_param_type(p.type) == KIND.EVENT_WRITER then
                emit_event_writer_setup_line(output, p)
            end
        end
    end

    if system_has_command_buffer_param(system) and not has_writer then
        table.insert(output, "\tpulse::command_buffer command_buffer(world);")
    end
    emit_singletons_and_writers()
    if system_has_command_buffer_param(system) and has_writer then
        table.insert(output, "\tpulse::command_buffer command_buffer(world);")
    end

    if inner_each then
        local sp = sig_params[1]
        local qinner = extract_template_type(sp.type)
        if not qinner then
            error("`inner_each`: 错误的查询类型 " .. sp.type)
        end
        local comp_param = first_component_param(system)
        local lam_arg = comp_param and comp_param.name or "elem"
        table.insert(output, string.format("\t%s.each([&](%s& %s)", sp.name, qinner, lam_arg))
        table.insert(output, "\t\t{")
        table.insert(output, string.format("\t\t\t%s(%s, %s);", system.name, er_name, lam_arg))
        table.insert(output, "\t\t});")
    else
        local call_args = {}
        for _, p in ipairs(system.params) do
            local k = classify_param_type(p.type)
            if k == KIND.EVENT_READER then
                table.insert(call_args, er_name)
            elseif k == KIND.COMMAND_BUFFER then
                table.insert(call_args, "command_buffer")
            elseif k == KIND.EVENT_WRITER then
                table.insert(call_args, event_writer_var_name(p))
            elseif k == KIND.SINGLETON_QUERY then
                table.insert(call_args, singleton_query_local_name(p))
            elseif k == KIND.FLECS_QUERY then
                table.insert(call_args, p.name)
            elseif k == KIND.COMPONENT then
                table.insert(call_args, p.name)
            end
        end
        local arg_str = table.concat(call_args, ", ")
        table.insert(output, string.format("\t%s(%s);", system.name, arg_str))
    end
    table.insert(output, "}")

    return table.concat(output, "\n")
end

local function get_register_name(system_name)
    local name = system_name:gsub("System$", "")
    local words = {}
    local current_word = ""
    for i = 1, #name do
        local c = name:sub(i, i)
        if c:match("%u") and current_word ~= "" then
            table.insert(words, current_word)
            current_word = c
        else
            current_word = current_word .. c
        end
    end
    if current_word ~= "" then
        table.insert(words, current_word)
    end

    local result = ""
    for i, word in ipairs(words) do
        if i == 1 then
            result = result .. word:sub(1, 1):upper() .. word:sub(2):lower()
        else
            result = result .. word:sub(1, 1):upper() .. word:sub(2):lower()
        end
    end
    return result
end

-- 当事件目标为实体且不使用内部 `query.each` 时，`EntityEventRegister` 的模板组件。
local function entity_event_register_main_components(sys_list)
    for _, sys in ipairs(sys_list) do
        if sys.entity_system and sys.has_event_reader and not event_entity_uses_inner_each(sys) and #sys.main_query_components > 0 then
            return table.concat(sys.main_query_components, ", ")
        end
    end
    return nil
end

local function emit_attached_query_system_registration(output, sys, pipeline_field)
    local reg_name = get_register_name(sys.name)
    local wn = wrapper_function_name(sys)
    local state_name = wrapper_state_struct_name(sys)
    local use_each = attached_query_registration_uses_each(sys)
    if use_each then
        local mq = sys.main_query_components
        if #mq > 0 then
            table.insert(output, string.format('\tauto %s = moduleContext->world.system<%s>("%s")', sys.name, table.concat(mq, ", "), reg_name))
        else
            table.insert(output, string.format('\tauto %s = moduleContext->world.system("%s")', sys.name, reg_name))
        end
    else
        table.insert(output, string.format('\tauto %s = moduleContext->world.system("%s")', sys.name, reg_name))
    end
    table.insert(output, "\t\t.kind(moduleContext->" .. pipeline_field .. ")")
    if sys.immediate then
        table.insert(output, "\t\t.immediate()")
    end
    if use_each then
        table.insert(output, string.format("\t\t.each(%s);", wn))
    else
        table.insert(output, string.format("\t\t.run(%s);", wn))
    end
    local set_fields = {}
    for qi, q in ipairs(sys.extra_queries) do
        local fname = attached_query_field_name(q, qi)
        local ct = normalize_type_for_registration(q.type)
        table.insert(set_fields, string.format(".%s = moduleContext->world.query<%s>()", fname, ct))
    end
    table.insert(output, string.format("\t%s.set<%s>({ %s });", sys.name, state_name, table.concat(set_fields, ", ")))
end

-- 普通 system 按相同时机注册（init / update / imgui），仅 pipeline 字段不同。
local function emit_simple_pipeline_registrations(output, phase_systems, pipeline_field)
    for _, sys in ipairs(phase_systems) do
        if system_needs_attached_query_state(sys) then
            emit_attached_query_system_registration(output, sys, pipeline_field)
        else
            local reg_name = get_register_name(sys.name)
            local wn = wrapper_function_name(sys)
            local mq = sys.main_query_components
            if #mq > 0 then
                table.insert(output, string.format('\tmoduleContext->world.system<%s>("%s")', table.concat(mq, ", "), reg_name))
            else
                table.insert(output, string.format('\tmoduleContext->world.system("%s")', reg_name))
            end
            table.insert(output, "\t\t.kind(moduleContext->" .. pipeline_field .. ")")
            if sys.immediate then
                table.insert(output, "\t\t.immediate()")
            end
            if sys.entity_system then
                table.insert(output, "\t\t.each(" .. wn .. ");")
            else
                table.insert(output, "\t\t.run(" .. wn .. ");")
            end
        end
        table.insert(output, "")
    end
end

local function generate_module_registration(systems)
    local output = {}

    local phase_buckets = {
        [PHASE.INIT] = {},
        [PHASE.UPDATE] = {},
        [PHASE.IMGUI] = {},
    }
    local event_systems = {}

    for _, sys in ipairs(systems) do
        if sys.has_event_reader then
            table.insert(event_systems, sys)
        elseif phase_buckets[sys.phase] then
            table.insert(phase_buckets[sys.phase], sys)
        end
    end

    for _, ph in ipairs(PHASE_ORDER) do
        emit_simple_pipeline_registrations(output, phase_buckets[ph], PIPELINE_FIELD[ph])
    end

    local event_groups = {}
    for _, sys in ipairs(event_systems) do
        if not event_groups[sys.event_type] then
            event_groups[sys.event_type] = {}
        end
        table.insert(event_groups[sys.event_type], sys)
    end

    local event_type_order = {}
    local seen_et = {}
    for _, sys in ipairs(systems) do
        if sys.has_event_reader and not seen_et[sys.event_type] then
            seen_et[sys.event_type] = true
            table.insert(event_type_order, sys.event_type)
        end
    end

    for _, event_type in ipairs(event_type_order) do
        local sys_list = event_groups[event_type]
        local disp_base = event_type:gsub("Event$", "")
        local dispatcher_name = disp_base:sub(1, 1):lower() .. disp_base:sub(2) .. "Dispatcher"
        local main_comps = entity_event_register_main_components(sys_list)

        if main_comps then
            table.insert(output, string.format("\tauto %s = std::make_unique<pulse::EntityEventRegister<%s, %s>>();", dispatcher_name, event_type, main_comps))
        else
            table.insert(output, string.format("\tauto %s = std::make_unique<pulse::EventRegister<%s>>();", dispatcher_name, event_type))
        end

        for _, sys in ipairs(sys_list) do
            local query_args = {}
            local seen_queries = {}
            for _, p in ipairs(sys.params) do
                if classify_param_type(p.type) == KIND.FLECS_QUERY then
                    local ct = normalize_type_for_registration(p.type)
                    if not seen_queries[ct] then
                        seen_queries[ct] = true
                        table.insert(query_args, "moduleContext->world.query<" .. ct .. ">()")
                    end
                end
            end
            if event_entity_uses_inner_each(sys) and #sys.main_query_components == 1 then
                local ct = sys.main_query_components[1]
                if not seen_queries[ct] then
                    seen_queries[ct] = true
                    table.insert(query_args, "moduleContext->world.query<" .. ct .. ">()")
                end
            end
            local wn = wrapper_function_name(sys)
            if #query_args > 0 then
                table.insert(output, string.format("\t%s->reg(%s%s);", dispatcher_name, wn, ", " .. table.concat(query_args, ", ")))
            else
                table.insert(output, string.format("\t%s->reg(%s);", dispatcher_name, wn))
            end
        end

        table.insert(output, string.format("\t%s->observe(moduleContext->world);", dispatcher_name))
        table.insert(output, string.format("\tmoduleContext->eventManager->register_event(std::move(%s));", dispatcher_name))
        table.insert(output, "")
    end

    return table.concat(output, "\n")
end

local function generate_module_cpp(module_name, systems)
    local output = {}

    table.insert(output, "#include \"" .. module_name .. "_module.h\"")
    table.insert(output, "#include \"" .. module_name .. ".h\"")
    table.insert(output, "")

    for _, sys in ipairs(systems) do
        if sys.has_event_reader then
            local wrapper = generate_event_wrapper(sys)
            if wrapper then
                for line in wrapper:gmatch("[^\n]+") do
                    table.insert(output, line)
                end
            end
        else
            if system_needs_attached_query_state(sys) then
                local st = generate_attached_query_state_struct(sys)
                for line in st:gmatch("[^\n]+") do
                    table.insert(output, line)
                end
            end
            local wrapper = generate_wrapper(sys)
            if wrapper then
                for line in wrapper:gmatch("[^\n]+") do
                    table.insert(output, line)
                end
            end
        end
    end

    table.insert(output, "")
    table.insert(output, "void importModule(pulse::ModuleContext* moduleContext)")
    table.insert(output, "{")

    local registrations = generate_module_registration(systems)
    for line in registrations:gmatch("[^\n]+") do
        table.insert(output, line)
    end

    table.insert(output, "}")

    return table.concat(output, "\n")
end

local function generate_module_h()
    return [[#pragma once

#include "module_importer.h"

void importModule(pulse::ModuleContext* moduleContext);
]]
end

local function print_usage()
    io.stderr:write(
        "用法: lua generate_module.lua <dump|generate> [头文件名.h]\n"
            .. "  默认头文件: snake.h | dump: 仅输出到控制台 | generate: 生成 <文件名>_module.{h,cpp}\n"
            .. "  旧版用法: lua generate_module.lua foo.h => generate foo.h\n"
    )
end

local function main(...)
    local argv = {...}
    local mode
    local header_path

    if #argv == 0 then
        print_usage()
        error("缺少模式或头文件路径")
    end

    local a1 = argv[1]
    if a1 == "dump" or a1 == "generate" then
        mode = a1
        header_path = argv[2] or "snake.h"
    elseif a1:match("%.h$") or a1:find("/", 1, true) or a1:find("\\", 1, true) then
        mode = "generate"
        header_path = a1
    else
        print_usage()
        error("需要 dump, generate, 或 .h 文件路径")
    end

    print("解析: " .. header_path)
    local content = read_file(header_path)
    local systems = parse_systems(content)

    if mode == "dump" then
        print_parsed_systems_dump(systems)
        print("完成 (dump 模式)。")
        return
    end

    local module_name = header_path:match("([^/\\]+)%.h$") or "module"
    module_name = module_name:gsub("%.h$", "")

    local module_h = generate_module_h()
    write_file(module_name .. "_module.h", module_h)
    print("输出: " .. module_name .. "_module.h")

    local module_cpp = generate_module_cpp(module_name, systems)
    write_file(module_name .. "_module.cpp", module_cpp)
    print("输出: " .. module_name .. "_module.cpp")

    print("完成 (generate 模式)。")
end

main(...)