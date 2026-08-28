--!Key step: compare the actual screenshot with the baseline and return similarity.

function main(baseline, actual, tolerance)
    local run_powershell = import("xmake.rules.window_common.run_powershell", {rootdir = os.projectdir(), anonymous = true})
    local scriptfile = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "compare_image.ps1")
    local outfile = os.tmpfile()
    local errfile = os.tmpfile()

    local envs = {
        PULSE_CMP_BASELINE  = baseline,
        PULSE_CMP_ACTUAL    = actual,
        PULSE_CMP_TOLERANCE = tostring(tolerance)
    }

    local res = run_powershell(scriptfile, envs, {outfile = outfile, errfile = errfile})
    if res and res.ok == 0 then
        local similarity = res.outdata:match("SIMILARITY=([%d%.]+)")
        if similarity then
            return tonumber(similarity)
        end
    end

    local errmsg = (res and (res.outdata:match("ERROR=(.+)") or res.errdata or res.syserrors)) or "compare_screenshot failed"
    return nil, errmsg
end