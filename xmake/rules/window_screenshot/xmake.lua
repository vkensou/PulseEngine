--!A custom xmake rule for window/rendering screenshot tests.
--
-- The test flow is assembled from reusable key-step modules:
-- start_process -> wait -> capture_screenshot -> compare_screenshot -> close_process.

rule("pulse.window_screenshot_test")

    on_test(function (target, opt)

        -- This screenshot flow is currently implemented for Windows only.
        if os.host() ~= "windows" then
            opt.errors = "当前非windows，不支持"
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        local start_process     = import(".window_common.start_process", {anonymous = true})
        local capture_screenshot = import(".window_common.capture_screenshot", {anonymous = true})
        local compare_screenshot = import(".window_common.compare_screenshot", {anonymous = true})
        local close_process      = import(".window_common.close_process", {anonymous = true})

        local rulename = "pulse.window_screenshot_test"
        local wait = tonumber(target:extraconf("rules", rulename, "wait") or 5)
        local threshold = tonumber(target:extraconf("rules", rulename, "threshold") or 0.95)
        local tolerance = tonumber(target:extraconf("rules", rulename, "tolerance") or 8)

        -- Baseline screenshot. The default location is tests/<module>/baseline.png.
        local baseline = target:extraconf("rules", rulename, "baseline")
        if not baseline then
            local module = target:name():gsub("^test%-", "")
            baseline = path.join(os.projectdir(), "tests", module, "baseline.png")
        end
        baseline = path.absolute(baseline, os.projectdir())

        -- Actual screenshot is emitted below build/tests so failures are easy to inspect.
        local shotdir = target:extraconf("rules", rulename, "outputdir")
        if not shotdir then
            shotdir = path.join(os.projectdir(), "build", "tests", "window_screenshot")
        end
        shotdir = path.absolute(shotdir, os.projectdir())
        local shotname = opt.name:gsub("[/\\]", "_") .. ".png"
        local actualshot = path.join(shotdir, shotname)

        -- Check baseline first: never launch the program when the reference is missing.
        if not os.isfile(baseline) then
            local hint = string.format(
                "缺少基准截图: %s\n请先用 .agents/skills/test-tool-for-program-with-window 生成客户区截图，"
                .. "并保存到该路径后重试。例如:\n"
                .. "python .agents/skills/test-tool-for-program-with-window/scripts/run_exe_wait_screenshot.py"
                .. " $(projectdir) %s %s %s --no-frame",
                baseline,
                path.absolute(target:targetfile(), os.projectdir()),
                wait,
                baseline)
            opt.errors = hint
            cprint("${color.failure}%s${clear}", hint)
            return false
        end

        local targetfile = path.absolute(target:targetfile(), os.projectdir())
        local rundir = opt.rundir or target:rundir()
        local runargs = table.wrap(opt.runargs or target:get("runargs"))

        -- Reuse xmake's built-in run environment (on Windows it collects the
        -- targetdirs of all shared deps into PATH), so the spawned exe can
        -- find pulse_*.dll even when package_output moves them into packages/.
        local runenvs = import("private.action.run.runenvs", {anonymous = true, try = true})
        local extrapath = ""
        if runenvs then
            local addenvs, setenvs = runenvs.make(target)
            local envs = runenvs.join(addenvs, setenvs)
            if envs and envs.PATH then
                extrapath = envs.PATH
            end
        end

        -- Key steps: launch, wait, capture, compare, close.
        local pid, start_err = start_process(targetfile, rundir, runargs, extrapath)
        if not pid then
            opt.errors = string.format("启动测试程序失败: %s", start_err)
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        os.sleep(wait * 1000)

        local ok, capture_err = capture_screenshot(pid, actualshot)
        if not ok then
            close_process(pid, 8000)
            opt.errors = string.format("窗口截图失败: %s\n实际截图: %s", capture_err, actualshot)
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        local similarity, cmp_err = compare_screenshot(baseline, actualshot, tolerance)
        close_process(pid, 8000)

        if not similarity then
            opt.errors = string.format("截图对比失败: %s\n实际截图: %s", cmp_err, actualshot)
            cprint("${color.failure}%s${clear}", opt.errors)
            return false
        end

        if similarity >= threshold then
            cprint("${color.success}screenshot test passed, similarity: %s${clear}", tostring(similarity))
            return true
        end

        opt.errors = string.format("截图相似度不足: %s (阈值: %s)\n实际截图: %s\n基准截图: %s",
            tostring(similarity), tostring(threshold), actualshot, baseline)
        cprint("${color.failure}%s${clear}", opt.errors)
        return false
    end)
