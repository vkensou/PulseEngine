-- generate_flecs_query.lua
-- 用于生成 flecs_query 模块的批量重载代码

local function generate_query_code(max_params)
    local output = {}
    
    table.insert(output, [[
options gen2

module flecs_query

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base

]])
    
    -- 生成 template_structure 定义
    for n = 1, max_params do
        local params = {}
        for i = 1, n do
            params[i] = "T" .. i
        end
        local param_list = table.concat(params, ", ")
        
        table.insert(output, string.format("[template_structure(%s)]\n", param_list))
        table.insert(output, string.format("struct template Query%d {\n", n))
        table.insert(output, "    query_: ecs_query_t?\n")
        table.insert(output, "}\n\n")
    end
    
    -- 生成 query 函数（创建查询）
    for n = 1, max_params do
        local params = {}
        local param_decls = {}
        for i = 1, n do
            params[i] = "t" .. i .. ": type<auto(T" .. i .. ")>"
            param_decls[i] = "t" .. i
        end
        local param_list = table.concat(params, "; ")
        local param_call = table.concat(param_decls, ", ")
        
        table.insert(output, string.format("def query(var world: World; %s) {\n", param_list))
        table.insert(output, "    var desc = ecs_query_desc_t();\n")
        
        -- 生成每个 term 的设置
        for i = 1, n do
            table.insert(output, string.format("    let inout%d = typeinfo is_const(type<T%d>) ? int16(1) : int16(0)\n", i, i))
            table.insert(output, string.format("    desc.terms[%d] = ecs_term_t(id = ecs_component(world, type<T%d>), inout = inout%d)\n", i-1, i, i))
        end
        
        table.insert(output, "    var query_ = build_query_from_desc(world, desc)\n")
        
        local template_params = {}
        for i = 1, n do
            template_params[i] = "T" .. i
        end
        local template_list = table.concat(template_params, ", ")
        
        table.insert(output, string.format("    typedef _Query%d = $Query%d<%s>\n", n, n, template_list))
        table.insert(output, string.format("    return _Query%d(query_ = query_)\n", n))
        table.insert(output, "}\n\n")
    end
    
    -- 生成 for_each 函数
    for n = 1, max_params do
        local params = {}
        for i = 1, n do
            params[i] = "t" .. i .. " : T" .. i
        end
        local param_list = table.concat(params, "; ")
        
        local template_params = {}
        for i = 1, n do
            template_params[i] = "auto(T" .. i .. ")"
        end
        local template_list = table.concat(template_params, ", ")
        
        local block_params = {}
        for i = 1, n do
            block_params[i] = "t" .. i .. " : T" .. i
        end
        local block_list = table.concat(block_params, "; ")
        
        table.insert(output, string.format("def for_each(query: $Query%d<%s>, blk : block<(entity: Entity; %s) : void>) {\n", n, template_list, block_list))
        table.insert(output, "    unsafe {\n")
        table.insert(output, "        var iter = query_iter(query.query_)\n")
        table.insert(output, "        var iter_ptr = addr(iter)\n")
        table.insert(output, "        while (iter_next(iter_ptr)) {\n")
        
        -- 生成字段提取
        for i = 1, n do
            table.insert(output, string.format("            var field%d = reinterpret<T%d -const?> iter_field(iter_ptr, typeinfo sizeof(type<T%d>), %d);\n", i, i, i, i-1))
        end
        
        table.insert(output, "            for (i in 0..iter_ptr.count) {\n")
        
        local blk_args = {"iter_entity(iter_ptr, i)"}
        for i = 1, n do
            blk_args[i+1] = "field" .. i .. "[i]"
        end
        local blk_call = table.concat(blk_args, ", ")
        
        table.insert(output, string.format("                blk(%s)\n", blk_call))
        table.insert(output, "            }\n")
        table.insert(output, "        }\n")
        table.insert(output, "    }\n")
        table.insert(output, "}\n\n")
    end
    
    -- 生成 first 函数
    for n = 1, max_params do
        local params = {}
        for i = 1, n do
            params[i] = "auto(T" .. i .. ")"
        end
        local template_list = table.concat(params, ", ")
        
        local return_types = {"Entity"}
        for i = 1, n do
            return_types[i+1] = "T" .. i
        end
        local return_type_list = table.concat(return_types, ", ")
        
        local default_values = {"default<Entity>"}
        for i = 1, n do
            default_values[i+1] = "default<T" .. i .. ">"
        end
        local default_list = table.concat(default_values, ", ")
        
        table.insert(output, string.format("def first(query: $Query%d<%s>) {\n", n, template_list))
        table.insert(output, "    unsafe {\n")
        table.insert(output, "        var iter = query_iter(query.query_)\n")
        table.insert(output, "        var iter_ptr = addr(iter)\n")
        table.insert(output, "        if (iter_next(iter_ptr) && iter_ptr.count > 0) {\n")
        
        -- 生成字段提取
        for i = 1, n do
            table.insert(output, string.format("            var field%d = reinterpret<T%d -const?> iter_field(iter_ptr, typeinfo sizeof(type<T%d>), %d);\n", i, i, i, i-1))
        end
        
        local return_values = {"iter_entity(iter_ptr, 0)"}
        for i = 1, n do
            return_values[i+1] = "field" .. i .. "[0]"
        end
        local return_list = table.concat(return_values, ", ")
        
        table.insert(output, string.format("            return (%s)\n", return_list))
        table.insert(output, "        } finally {\n")
        table.insert(output, "            while(iter_next(iter_ptr)) {}\n")
        table.insert(output, "        }\n")
        table.insert(output, "    }\n")
        table.insert(output, string.format("    panic(\"there is no entity matched by this query!\")\n"))
        table.insert(output, string.format("    return (%s)\n", default_list))
        table.insert(output, "}\n\n")
    end
    
    return table.concat(output)
end

-- 配置：通过命令行参数设置
local MAX_PARAMS = tonumber(arg[1]) or 10  -- 第一个参数：最大参数数量，默认10
local output_file = arg[2] or "flecs_query_gen.das"  -- 第二个参数：输出文件路径，默认flecs_query_gen.das

-- 生成代码
local generated_code = generate_query_code(MAX_PARAMS)

-- 输出到文件
local file = io.open(output_file, "w")
if file then
    file:write(generated_code)
    file:close()
    print(string.format("代码已生成到 %s (MAX_PARAMS=%d)", output_file, MAX_PARAMS))
else
    print(string.format("错误：无法写入文件 %s", output_file))
    -- 如果无法写入文件，输出到控制台
    print(generated_code)
end