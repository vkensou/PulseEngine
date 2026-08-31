--!Shared infrastructure: run a single-purpose .ps1 script and capture its output.

function main(scriptfile, envs, opt)
    opt = opt or {}
    local find_tool = import("lib.detect.find_tool")
    local powershell = assert(find_tool("powershell"), "powershell not found!")
    local outfile = opt.outfile
    local errfile = opt.errfile
    if not outfile or not errfile then
        return nil, "run_powershell: outfile and errfile are required"
    end

    os.mkdir(path.directory(outfile))
    os.tryrm(outfile)
    os.tryrm(errfile)

    local ok, syserrors = os.execv(powershell.program,
        {"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", scriptfile},
        {try = true, curdir = opt.rundir, envs = envs, stdout = outfile, stderr = errfile})

    local outdata = (os.isfile(outfile) and io.readfile(outfile)) or ""
    local errdata = (os.isfile(errfile) and io.readfile(errfile)) or ""
    os.tryrm(outfile)
    os.tryrm(errfile)

    return {ok = ok, syserrors = syserrors, outdata = outdata, errdata = errdata}
end
