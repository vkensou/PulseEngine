--!Key step: wait until the process shows a top-level window with the expected title.

function main(pid, title, wait_seconds)
    local run_powershell = import("xmake.rules.window_common.run_powershell", {rootdir = os.projectdir(), anonymous = true})
    local scriptfile = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "wait_for_title.ps1")
    local outfile = os.tmpfile()
    local errfile = os.tmpfile()

    local envs = {
        PULSE_TITLE_PID  = tostring(pid),
        PULSE_TITLE      = title,
        PULSE_TITLE_WAIT = tostring(wait_seconds)
    }

    local res = run_powershell(scriptfile, envs, {outfile = outfile, errfile = errfile})
    if res and res.ok == 0 then
        return true, res.outdata:match("FOUND_TITLE=(.+)") or title
    end

    local errmsg = (res and (res.outdata:match("ERROR=(.+)") or res.errdata or res.syserrors)) or "wait_for_title failed"
    return false, errmsg
end