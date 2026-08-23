--!Key step: start the target executable and return its PID.

function main(targetfile, rundir, runargs)
    local json = import("core.base.json")
    local run_powershell = import("xmake.rules.window_common.run_powershell", {rootdir = os.projectdir(), anonymous = true})
    local scriptfile = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "start_process.ps1")
    local outfile = os.tmpfile()
    local errfile = os.tmpfile()

    local envs = {
        PULSE_PROC_EXE       = targetfile,
        PULSE_PROC_CURDIR    = rundir,
        PULSE_PROC_ARGS_JSON = (#runargs > 0) and json.encode(runargs) or "[]"
    }

    local res = run_powershell(scriptfile, envs, {outfile = outfile, errfile = errfile})
    if res and res.ok == 0 then
        local pid = res.outdata:match("PID=(%d+)")
        if pid then
            return tonumber(pid)
        end
    end

    local errmsg = (res and (res.outdata:match("ERROR=(.+)") or res.errdata or res.syserrors)) or "start_process failed"
    return nil, errmsg
end