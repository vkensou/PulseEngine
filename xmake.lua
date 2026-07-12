add_rules("mode.debug", "mode.release", "mode.releasedbg")
set_languages("cxx20", "c11")
set_exceptions("none")
if is_plat("windows") then 
    add_defines("NOMINMAX")
    set_runtimes(is_mode("debug") and "MDd" or "MD")
    add_ldflags("-subsystem:console")
elseif is_plat("android") then
    add_cxflags("-fPIC")
    includes("androidcpp")
    set_runtimes("c++_static")
end

add_cxxflags("/Zc:__cplusplus", "/Zc:preprocessor", {tools = "cl", force = true})

if is_host("windows") and is_plat("android") then
    set_policy("install.strip_packagelibs", false)
end

add_requires("libsdl3", {configs = {sdlmain = false, shared = is_plat("android")}})
add_requires("imgui v1.92.1-docking")
add_requires("mimalloc")
-- add_requires("flecs")
add_requires("enkits")

if is_plat("windows", "linux", "android") then
    option("use_vulkan")
        set_showmenu(true )
        set_default(true)
end

includes("cgpu/xmake.lua")

target("rendergraph")
    set_kind("static")
    add_deps("cgpu")
    add_includedirs("src/rendergraph/include", {public = true})
    add_headerfiles("src/rendergraph/include/*.h")
    add_headerfiles("src/rendergraph/src/*.h", {install = false})
    add_files("src/rendergraph/src/*.cpp")

includes("src/khr/xmake.lua")

target("rgframework")
    set_kind("static")
    add_deps("cgpu")
    add_deps("rendergraph")
    add_deps("ktx")
    add_defines("KHRONOS_STATIC")
    add_packages("libsdl3", {public = true})
    add_packages("imgui", {public = true})
    add_packages("mimalloc", {public = true})
    add_packages("enkits", {public = true})
    -- add_packages("flecs", {public = true})
    add_rules("utils.hlsl2spv", {bin2c = true})
    set_pcheader("src/rgframework/src/pcheader.h")
    add_includedirs("src/rgframework/include", {public = true})
    add_headerfiles("src/rgframework/include/*.h")
    add_headerfiles("src/rgframework/include/*.hpp")
    add_headerfiles("src/rgframework/src/*.h", {install = false})
    add_files("src/rgframework/src/*.cpp")
    add_files("src/rgframework/src/*.hlsl")
    add_files("src/rgframework/src/flecs.c")
    if is_plat("windows") then 
        add_syslinks("Advapi32")
    end 

target("pulse_app")
    set_kind("static")
    add_includedirs("src/pulse_app/include", {public = true})
    add_headerfiles("src/pulse_app/include/*.h")
    add_files("src/pulse_app/src/*.c")
    add_files("src/pulse_app/src/*.cpp")

target("pulse_math")
    set_kind("headeronly")
    add_includedirs("src/pulse_math/include", {public = true})
    add_headerfiles("src/pulse_math/include/*.h")

target("pulse_window")
    set_kind("static")
    add_deps("pulse_app")
    add_packages("libsdl3", {public = true})
    add_includedirs("src/pulse_window/include", {public = true})
    add_headerfiles("src/pulse_window/include/*.h")
    add_files("src/pulse_window/src/*.cpp")

target("pulse_input")
    set_kind("static")
    add_deps("pulse_app")
    add_deps("pulse_window")
    add_includedirs("src/pulse_input/include", {public = true})
    add_headerfiles("src/pulse_input/include/*.h")
    add_files("src/pulse_input/src/*.cpp")

target("pulse_asset")
    set_kind("static")
    add_deps("pulse_app")
    add_packages("libsdl3", {public = true})
    add_includedirs("src/pulse_asset/include", {public = true})
    add_headerfiles("src/pulse_asset/include/*.h")
    add_files("src/pulse_asset/src/*.cpp")

target("pulse_graphics")
    set_kind("static")
    set_exceptions("cxx")
    add_deps("pulse_app")
    add_deps("pulse_window")
    add_deps("pulse_asset")
    add_deps("cgpu", {public = true})
    add_deps("rendergraph", {public = true})
    add_deps("ktx")
    add_defines("KHRONOS_STATIC")
    add_includedirs("src/pulse_graphics/include", {public = true})
    add_headerfiles("src/pulse_graphics/include/*.h")
    add_headerfiles("src/pulse_graphics/src/*.h", {install = false})
    add_files("src/pulse_graphics/src/*.cpp")
    add_files("src/pulse_graphics/src/**/*.cpp")
    add_rules("utils.hlsl2spv", {bin2c = true})
    add_files("src/pulse_graphics/src/runtime/*.hlsl")

rule("example_base")
    after_load(function(target)
        target:set("group", "examples")
        if is_plat("android") then
            target:set("kind", "shared")
        else 
            target:set("kind", "binary")
            if is_plat("windows") then
                target:add("ldflags", "/subsystem:console")
            end
        end
        target:set("rundir", "$(projectdir)/examples/assets")
        target:add("deps", "rgframework")
    end)

target("rendersystem")
    add_rules("example_base")
    if is_plat("android") then
        add_rules("androidcpp", {android_sdk_version = "34", android_manifest = "examples/AndroidManifest.xml", android_res = "examples/res", android_assets = "examples/assets", attachedjar = path.join("androidsdl", "libsdl3-3.2.16.jar"), apk_output_path = ".", package_name = "com.xmake.androidcpp", activity_name = "org.libsdl.app.SDLActivity"})
    end
    add_files("examples/rendersystem/*.cpp")

target("snake")
    add_rules("example_base")
    if is_plat("android") then
        add_rules("androidcpp", {android_sdk_version = "34", android_manifest = "examples/AndroidManifest.xml", android_res = "examples/res", android_assets = "examples/assets", attachedjar = path.join("androidsdl", "libsdl3-3.2.16.jar"), apk_output_path = ".", package_name = "com.xmake.androidcpp", activity_name = "org.libsdl.app.SDLActivity"})
    end
    add_files("examples/snake/*.cpp")

includes("dascript/xmake.lua")

target("das-example")
    add_rules("example_base")
    add_files("examples/dascript/*.cpp")
    add_files("examples/dascript/dasCGPU/*.cpp")
    add_deps("libDaScript")

target("test-app")
    set_group("tests")
    set_kind("binary")
    set_rundir("$(projectdir)/examples/assets")
    add_deps("pulse_app")
    add_files("tests/app/*.cpp")

target("test-window")
    set_group("tests")
    set_kind("binary")
    set_rundir("$(projectdir)/examples/assets")
    add_deps("pulse_app")
    add_deps("pulse_window")
    add_files("tests/window/*.cpp")

target("test-asset")
    set_group("tests")
    set_kind("binary")
    set_rundir("$(projectdir)")
    add_deps("pulse_app")
    add_deps("pulse_asset")
    add_files("tests/asset/*.cpp")

target("test-graphics")
    set_group("tests")
    set_kind("binary")
    set_rundir("$(projectdir)")
    add_deps("pulse_app")
    add_deps("pulse_window")
    add_deps("pulse_asset")
    add_deps("pulse_graphics")
    add_files("tests/graphics/*.cpp")

target("test-math")
    set_group("tests")
    set_kind("binary")
    add_deps("pulse_math")
    add_files("tests/math/*.cpp")

target("test-input")
    set_group("tests")
    set_kind("binary")
    set_rundir("$(projectdir)/examples/assets")
    add_deps("pulse_app")
    add_deps("pulse_input")
    add_files("tests/input/*.cpp")
