if is_plat("windows") then
    add_defines("DAS_ENABLE_EXCEPTIONS=1")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_cxflags("/utf-8", "/bigobj")
    add_mxflags("/MP")
elseif is_plat("unix") then
    add_cxflags("-Wno-strict-aliasing")
end

-- add_requires("Threads")


target("libUriParser")
    set_kind("static")
    add_files("3rdparty/uriparser/src/*.c")
    add_includedirs("3rdparty/uriparser/include")
    add_defines("URIPARSER_BUILD_CHAR", "URI_STATIC_BUILD")

target("libDaScript")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_includedirs("3rdparty/fmt/include")
    add_includedirs("3rdparty/uriparser/include")
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
    add_files("3rdparty/fmt/src/format.cc")
    add_deps("libUriParser")
    if is_plat("windows") then
        add_links("dbghelp")
    elseif is_plat("linux") or is_plat("macosx") then
        add_links("dl")
    end

