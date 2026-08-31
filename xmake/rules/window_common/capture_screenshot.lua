--!Key step: capture the client area of the process window to a PNG file.

function main(pid, output_path)
    local run_powershell = import("xmake.rules.window_common.run_powershell", {rootdir = os.projectdir(), anonymous = true})
    local scriptfile = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "capture_client.ps1")
    os.mkdir(path.directory(output_path))

    local outfile = os.tmpfile()
    local errfile = os.tmpfile()

    local envs = {
        PULSE_CAPTURE_NATIVE = path.join(os.projectdir(), "xmake", "rules", "window_common", "scripts", "native.ps1"),
        PULSE_CAPTURE_PID    = tostring(pid),
        PULSE_CAPTURE_SHOT   = output_path
    }

    local res = run_powershell(scriptfile, envs, {outfile = outfile, errfile = errfile})
    if res and res.ok == 0 then
        return true
    end

    local errmsg = (res and (res.outdata:match("ERROR=(.+)") or res.errdata or res.syserrors)) or "capture_screenshot failed"
    return nil, errmsg
end