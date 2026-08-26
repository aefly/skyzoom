includes("lib/commonlibsse-ng")

set_project("skyzoom")
set_version("1.0.0")
set_license("MIT")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

target("SkyZoom")
add_rules("commonlibsse-ng.plugin", {
    name = "SkyZoom",
    author = "aefly",
    description = "Simple hold-to-zoom feature for Skyrim SE/AE"
})
add_deps("commonlibsse-ng")

add_files("src/**.cpp")
add_headerfiles("src/**.h")
add_includedirs("src")
set_pcxxheader("src/PCH.h")

-- xinput9_1_0 is present on every Windows version and forwards to the installed runtime
add_syslinks("dxgi", "user32", "xinput9_1_0")

add_installfiles("dist/SkyZoom.ini", { prefixdir = "SKSE/Plugins" })
add_installfiles("dist/SkyZoom.esp", { prefixdir = "" })
