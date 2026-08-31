--!Key step: send WM_CLOSE to the process, then force-kill if it does not exit.

function main(pid, close_ms)
    local run_powershell = import("xmake.rules.window_common.run_powershell", {rootdir = os.projectdir(), anonymous = true})
    local scriptfile = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "close_process.ps1")
    local outfile = os.tmpfile()
    local errfile = os.tmpfile()

    local envs = {
        PULSE_CLOSE_NATIVE = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "native.ps1"),
        PULSE_CLOSE_PID    = tostring(pid),
        PULSE_CLOSE_MS     = tostring(close_ms or 8000)
    }

    local res = run_powershell(scriptfile, envs, {outfile = outfile, errfile = errfile})
    if res and res.ok == 0 then
        local force_killed = res.outdata:find("FORCE_KILLED=1", 1, true) ~= nil
        return true, force_killed
    end

    local errmsg = (res and (res.outdata:match("ERROR=(.+)") or res.errdata or res.syserrors)) or "close_process failed"
    return nil, errmsg
end