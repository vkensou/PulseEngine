--!A custom xmake rule to publish Pulse packages after build.
--
-- The rule sets each target's targetdir directly to the desired output folder:
--
--     build/out/<plat>/<mode>/packages/<target-name>/
--
-- Core/runtime targets (pulse_app, pulse_package_loader, launcher) can opt out
-- of the packages/ subdirectory with:
--
--     add_rules("pulse.package_output", {location = "root"})
--
-- They are then built directly into:
--
--     build/out/<plat>/<mode>/
--
-- After build, the rule only copies the target's own manifest (package.json or
-- packages.json) next to the built artifact. Optional extra resource directories
-- can be configured with extra_dirs.
--

rule("pulse.package_output")

    on_load(function (target)
        -- Only shared libraries and executables are published by this rule.
        if not (target:is_shared() or target:is_binary()) then
            return
        end

        local plat = is_plat("windows") and "windows" or is_plat("linux") and "linux"
        if not plat then
            return
        end

        local mode = is_mode("debug") and "debug" or "release"
        local rulename = "pulse.package_output"
        local output_root = path.join(os.projectdir(), "build", plat, mode)

        local location = target:extraconf("rules", rulename, "location")
        local targetdir
        if location == "root" then
            targetdir = output_root
        else
            targetdir = path.join(output_root, "packages", target:name())
        end

        os.mkdir(targetdir)
        target:set("targetdir", targetdir)
        if target:is_binary() then
            target:set("rundir", targetdir)
        end
    end)

    after_build(function (target)
        -- Only shared libraries and executables are published by this rule.
        if not (target:is_shared() or target:is_binary()) then
            return
        end

        local rulename = "pulse.package_output"

        -- Copy the target's own manifest next to the built artifact.
        local manifest = target:extraconf("rules", rulename, "manifest")
        if manifest then
            manifest = path.absolute(manifest, os.projectdir())
        else
            local candidates = {}
            local sourcefiles = target:sourcefiles()
            if #sourcefiles > 0 then
                local dir = path.directory(sourcefiles[1])
                table.insert(candidates, path.join(dir, "package.json"))
                table.insert(candidates, path.join(path.directory(dir), "package.json"))
                table.insert(candidates, path.join(dir, "packages.json"))
                table.insert(candidates, path.join(path.directory(dir), "packages.json"))
            end

            local name = target:name()
            table.insert(candidates, path.join(os.projectdir(), "src", name, "package.json"))
            table.insert(candidates, path.join(os.projectdir(), "examples", name, "package.json"))
            table.insert(candidates, path.join(os.projectdir(), "examples", name:gsub("^example%-", ""), "package.json"))
            table.insert(candidates, path.join(os.projectdir(), "examples", name, "packages.json"))

            for _, file in ipairs(candidates) do
                if os.isfile(file) then
                    manifest = file
                    break
                end
            end
        end

        if manifest and os.isfile(manifest) then
            os.cp(manifest, path.join(target:targetdir(), path.filename(manifest)))
        end

        -- Optional extra resource directories (e.g. snake assets).
        local extra_dirs = target:extraconf("rules", rulename, "extra_dirs")
        for _, dir in ipairs(table.wrap(extra_dirs)) do
            dir = path.absolute(dir, os.projectdir())
            if os.isdir(dir) then
                os.cp(path.join(dir, "*"), target:targetdir())
            end
        end
    end)