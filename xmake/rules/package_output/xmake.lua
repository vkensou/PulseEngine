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

rule("pulse.copy_manifest")

    after_build(function (target)

        if not (target:is_binary()) then
            return
        end

        local rulename = "pulse.copy_manifest"
        local location = target:extraconf("rules", rulename, "manifest")

        if location and os.isfile(location) then
            os.cp(location, path.join(target:targetdir(), path.filename(location)))
        end
    end)

-- Copies runtime libraries from xmake packages into the target's output directory.
-- Usage:
--   add_rules("pulse.copy_package_runtime", {packages = {"imgui"}})
--   add_rules("pulse.copy_package_runtime", {packages = {"imgui"}, files = {"imgui.dll"}})
rule("pulse.copy_package_runtime")

    after_build(function (target)
        -- Only shared libraries and executables are published by this rule.
        if not (target:is_shared() or target:is_binary()) then
            return
        end

        local rulename = "pulse.copy_package_runtime"
        local packages = target:extraconf("rules", rulename, "packages")
        local files = target:extraconf("rules", rulename, "files")
        local project = import("core.project.project")

        for _, pkgname in ipairs(table.wrap(packages)) do
            local pkg = target:pkg(pkgname)
            if not pkg then
                pkg = project.required_package(pkgname)
            end
            if pkg then
                local destdir = target:targetdir()
                local libfiles = pkg:get("libfiles") or {}
                local selected = table.wrap(files)
                local has_filter = #selected > 0

                for _, file in ipairs(libfiles) do
                    local filename = path.filename(file)
                    local matched = false

                    if has_filter then
                        for _, wanted in ipairs(selected) do
                            if filename == wanted then
                                matched = true
                                break
                            end
                        end
                    else
                        if filename:find("%.dll") or filename:find("%.so") or filename:find("%.dylib") then
                            matched = true
                        end
                    end

                    if matched then
                        os.cp(file, path.join(destdir, filename))
                    end
                end
            end
        end
    end)

rule("pulse.package_install")

    on_install(function (target)

        if not (target:is_shared() or target:is_binary()) then
            return
        end

        local rulename = "pulse.package_install"
        local manifest = target:extraconf("rules", rulename, "manifest")

        local destdir = target:installdir()
        if not manifest then
            os.raise("pulse.package_install: manifest not configured")
        end
        if not destdir then
            os.raise("pulse.package_install: installdir not set")
        end

        manifest = path.absolute(manifest, os.projectdir())
        if not os.isfile(manifest) then
            os.raise("pulse.package_install: manifest not found: %s", manifest)
        end

        os.mkdir(destdir)

        local targetfile = target:targetfile()
        if targetfile and os.isfile(targetfile) then
            os.cp(targetfile, destdir)
        end

        os.cp(manifest, destdir)

        local json = import("core.base.json")
        local manifest_data = json.decode(io.readfile(manifest))
        if not manifest_data then
            os.raise("pulse.package_install: failed to parse manifest: %s", manifest)
        end

        local manifestdir = path.directory(manifest)
        for _, relpath in ipairs(table.wrap(manifest_data.paths)) do
            local srcpath = path.join(manifestdir, relpath)
            if not (os.isfile(srcpath) or os.isdir(srcpath)) then
                os.raise("pulse.package_install: resource not found: %s", srcpath)
            end
            os.cp(srcpath, destdir)
        end

        for _, pkg in ipairs(target:orderpkgs()) do
            for _, file in ipairs(pkg:get("libfiles") or {}) do
                local filename = path.filename(file)
                if filename:find("%.dll") or filename:find("%.so") or filename:find("%.dylib") then
                    os.cp(file, path.join(destdir, filename))
                end
            end
        end
    end)
