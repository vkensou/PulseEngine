--!A custom xmake rule for window-title smoke tests.
--
-- The test flow is assembled from reusable key-step modules:
-- start_process -> wait_for_title -> close_process.

rule("pulse.window_title_test")

    on_test(function (target, opt)

        -- This title-test flow is currently implemented for Windows only.
        if os.host() ~= "windows" then
            opt.errors = "当前非windows，不支持"
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        local start_process  = import(".window_common.start_process", {anonymous = true})
        local wait_for_title = import(".window_common.wait_for_title", {anonymous = true})
        local close_process  = import(".window_common.close_process", {anonymous = true})

        local rulename = "pulse.window_title_test"
        local title = target:extraconf("rules", rulename, "title") or "test-window retitled"
        local wait = tonumber(target:extraconf("rules", rulename, "wait") or 10)
        local close_ms = tonumber(target:extraconf("rules", rulename, "close_timeout_ms") or 8000)

        local targetfile = path.absolute(target:targetfile(), os.projectdir())
        local rundir = opt.rundir or target:rundir()
        local runargs = table.wrap(opt.runargs or target:get("runargs"))

        -- Key steps: launch, wait for the expected title, then close.
        local pid, start_err = start_process(targetfile, rundir, runargs)
        if not pid then
            opt.errors = string.format("启动测试程序失败: %s", start_err)
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        local found, title_err = wait_for_title(pid, title, wait)
        close_process(pid, close_ms)

        if found then
            cprint("${color.success}window title test passed: %s${clear}", title_err or "ok")
            return true
        end

        opt.errors = string.format("窗口标题测试失败: %s", title_err or "未知错误")
        cprint("${color.failure}%s${clear}", opt.errors)
        return false
    end)