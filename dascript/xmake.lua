
add_requires("fmt <12.0.0")
add_requires("uriparser")

target("libDaScript")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_includedirs("include/daScript")
    add_includedirs("include/vecmath")
    add_includedirs("include/ska")
    add_includedirs("include/fast_float")
    add_includedirs("include/dag_noise")
    add_files("src/parser/*.cpp")
    add_files("src/ast/*.cpp")
    add_files("src/simulate/*.cpp")
    add_files("src/hal/*.cpp")
    add_files("src/misc/*.cpp")
    add_files("src/builtin/*.cpp")
    add_packages("fmt")
    add_packages("uriparser")
    if is_plat("windows") then
        add_defines("DAS_ENABLE_EXCEPTIONS=1")
        add_defines("_CRT_SECURE_NO_WARNINGS")
        add_cxflags("/utf-8", "/bigobj")
        add_mxflags("/MP")
        add_links("dbghelp")
    elseif is_plat("unix") then
        add_cxflags("-Wno-strict-aliasing")
        add_links("dl")
    end

