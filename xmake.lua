---------------------------------------------------------------
-- PROJECT CONFIGURATION
---------------------------------------------------------------

set_project("skyzoom")
set_version("1.4.0")
set_license("GPL-3.0-or-later")
set_languages("c++23")
set_warnings("allextra")
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

---------------------------------------------------------------
-- DEPENDENCIES
---------------------------------------------------------------

includes("lib/commonlibsse-ng")
add_requires("minhook")

---------------------------------------------------------------
-- VR SUPPORT
---------------------------------------------------------------

-- SkyZoom targets Skyrim SE/AE only. Keep VR disabled because
-- including openvr.h transitively pulls in the real <Windows.h>,
-- which conflicts with REX::W32::MAX_PATH by defining the same
-- name.

set_config("skyrim_vr", false)

---------------------------------------------------------------
-- CORE PLUGIN
---------------------------------------------------------------
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

-- Every Windows install ships xinput9_1_0, and it forwards
-- calls to whichever XInput runtime is actually present.
add_syslinks("dxgi", "user32", "xinput9_1_0")

add_installfiles("dist/SkyZoom.ini", { prefixdir = "SKSE/Plugins" })
add_installfiles("dist/SkyZoom.esp", { prefixdir = "" })

---------------------------------------------------------------
-- PATCHES
---------------------------------------------------------------

----- Improved Camera -----
target("SkyZoomICPatch")
set_default(false)
set_version("1.1.0")
add_rules("commonlibsse-ng.plugin", {
    name = "SkyZoomICPatch",
    author = "aefly",
    description = "SkyZoom compatibility patch for Improved Camera SE"
})
add_deps("commonlibsse-ng")
add_packages("minhook")
add_files("patch/improved-camera/src/**.cpp")
add_headerfiles("patch/improved-camera/src/**.h")
add_includedirs("patch/improved-camera/src")
set_pcxxheader("src/PCH.h")

----- FirstPersonFOVSKSE -----
target("SkyZoomFPFOVPatch")
set_default(false)
set_version("1.0.0")
add_rules("commonlibsse-ng.plugin", {
    name = "SkyZoomFPFOVPatch",
    author = "aefly",
    description = "SkyZoom compatibility patch for FirstPersonFOVSKSE"
})
add_deps("commonlibsse-ng")
add_packages("minhook")
add_files("patch/first-person-fov/src/**.cpp")
add_headerfiles("patch/first-person-fov/src/**.h")
add_includedirs("patch/first-person-fov/src")
set_pcxxheader("src/PCH.h")
