-- VR disabled: SkyZoom is SE/AE only, and pulling in openvr.h transitively
-- drags in the real <Windows.h> (breaking REX::W32::MAX_PATH, which expects
-- to be the only definition of that name).
set_config("skyrim_vr", false)

includes("lib/commonlibsse-ng")

set_project("skyzoom")
set_version("1.2.0")
set_license("GPL-3.0-or-later")
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
