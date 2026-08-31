-- 小型独立 JSON 解析器，供 generate_module.lua 读取 packageinfo/package.json 使用。
-- 支持 object / array / string / number / true / false / null；足够解析简单 manifest。

local json = {}

local function parse_json(text)
    local pos = 1

    local function skip_ws()
        while pos <= #text do
            local c = text:sub(pos, pos)
            if c == " " or c == "\t" or c == "\r" or c == "\n" then
                pos = pos + 1
            else
                break
            end
        end
    end

    local function fail(msg)
        error("json: JSON 解析失败: " .. msg .. " (位置 " .. pos .. ")")
    end

    local function parse_string()
        if text:sub(pos, pos) ~= '"' then
            fail("expected string")
        end
        pos = pos + 1
        local out = {}
        while pos <= #text do
            local c = text:sub(pos, pos)
            if c == '"' then
                pos = pos + 1
                return table.concat(out)
            elseif c == "\\" then
                pos = pos + 1
                local esc = text:sub(pos, pos)
                pos = pos + 1
                local map = {
                    ['"'] = '"',
                    ["\\"] = "\\",
                    ["/"] = "/",
                    ["b"] = "\b",
                    ["f"] = "\f",
                    ["n"] = "\n",
                    ["r"] = "\r",
                    ["t"] = "\t",
                }
                if map[esc] then
                    table.insert(out, map[esc])
                elseif esc == "u" then
                    local hex = text:sub(pos, pos + 3)
                    if #hex < 4 or not hex:match("^%x%x%x%x$") then
                        fail("invalid \\u escape")
                    end
                    pos = pos + 4
                    local cp = tonumber(hex, 16)
                    if cp < 0x80 then
                        table.insert(out, string.char(cp))
                    elseif cp < 0x800 then
                        table.insert(out, string.char(
                            0xC0 + math.floor(cp / 0x40),
                            0x80 + cp % 0x40
                        ))
                    else
                        table.insert(out, string.char(
                            0xE0 + math.floor(cp / 0x1000),
                            0x80 + math.floor(cp / 0x40) % 0x40,
                            0x80 + cp % 0x40
                        ))
                    end
                else
                    fail("invalid escape \\" .. esc)
                end
            else
                table.insert(out, c)
                pos = pos + 1
            end
        end
        fail("unterminated string")
    end

    local function parse_number()
        local start = pos
        if text:sub(pos, pos) == "-" then
            pos = pos + 1
        end
        while pos <= #text and text:sub(pos, pos):match("[0-9%.eE+%-]") do
            pos = pos + 1
        end
        local num = text:sub(start, pos - 1)
        if num == "" then
            fail("invalid number")
        end
        local n = tonumber(num)
        if n == nil then
            fail("invalid number: " .. num)
        end
        return n
    end

    local parse_value
    local function parse_array()
        if text:sub(pos, pos) ~= "[" then
            fail("expected [")
        end
        pos = pos + 1
        skip_ws()
        local arr = {}
        if text:sub(pos, pos) == "]" then
            pos = pos + 1
            return arr
        end
        while true do
            skip_ws()
            table.insert(arr, parse_value())
            skip_ws()
            local c = text:sub(pos, pos)
            if c == "," then
                pos = pos + 1
            elseif c == "]" then
                pos = pos + 1
                return arr
            else
                fail("expected , or ]")
            end
        end
    end

    local function parse_object()
        if text:sub(pos, pos) ~= "{" then
            fail("expected {")
        end
        pos = pos + 1
        skip_ws()
        local obj = {}
        if text:sub(pos, pos) == "}" then
            pos = pos + 1
            return obj
        end
        while true do
            skip_ws()
            local key = parse_string()
            skip_ws()
            if text:sub(pos, pos) ~= ":" then
                fail("expected :")
            end
            pos = pos + 1
            skip_ws()
            obj[key] = parse_value()
            skip_ws()
            local c = text:sub(pos, pos)
            if c == "," then
                pos = pos + 1
            elseif c == "}" then
                pos = pos + 1
                return obj
            else
                fail("expected , or }")
            end
        end
    end

    parse_value = function()
        skip_ws()
        local c = text:sub(pos, pos)
        if c == "{" then
            return parse_object()
        elseif c == "[" then
            return parse_array()
        elseif c == '"' then
            return parse_string()
        elseif text:sub(pos, pos + 3) == "true" then
            pos = pos + 4
            return true
        elseif text:sub(pos, pos + 4) == "false" then
            pos = pos + 5
            return false
        elseif text:sub(pos, pos + 3) == "null" then
            pos = pos + 4
            return nil
        else
            return parse_number()
        end
    end

    skip_ws()
    local result = parse_value()
    skip_ws()
    if pos <= #text then
        fail("trailing characters")
    end
    return result
end

json.parse = parse_json

return json