set_xmakever("3.0.0")
includes("extern/commonlibf4")
set_project("ENBInputDisablerFO4")
set_version("1.0.2")
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")
add_rules("mode.debug", "mode.releasedbg")

target("ENBInputDisablerFO4")
    add_rules("commonlibf4.plugin", {
        name = "ENBInputDisablerFO4",
        author = "",
        description = "Block Fallout 4 keyboard and mouse input while the ENB editor is open.",
        plugin_template = path.join(os.projectdir(), "res/commonlibf4-plugin.cpp.in"),
    })
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
    add_syslinks("psapi", "user32")

target("input-state-tests")
    set_kind("binary")
    set_default(false)
    add_files("tests/input-state.cpp")
    add_includedirs("src")
