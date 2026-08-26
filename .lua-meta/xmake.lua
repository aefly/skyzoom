---@meta
---@diagnostic disable

-- Stub declarations for xmake's project-description DSL: these functions are
-- injected into xmake.lua's execution sandbox by xmake itself at build time,
-- so a standalone Lua language server has no way to know they exist. This
-- file only exists to silence "undefined global" diagnostics while editing
-- xmake.lua/xmake project files - it has no runtime effect (xmake never
-- loads this directory) and does not attempt to describe real parameter or
-- return types.

-- project / target scope
function includes(...) end

function set_project(...) end

function set_version(...) end

function set_license(...) end

function set_languages(...) end

function set_warnings(...) end

function set_arch(...) end

function set_encodings(...) end

function set_xmakever(...) end

function set_kind(...) end

function set_default(...) end

function set_description(...) end

function set_pcxxheader(...) end

function add_rules(...) end

function add_files(...) end

function add_headerfiles(...) end

function add_includedirs(...) end

function add_installfiles(...) end

function add_syslinks(...) end

function add_deps(...) end

function add_defines(...) end

function add_cxxflags(...) end

function add_options(...) end

function add_packages(...) end

function add_requires(...) end

-- declaration scopes
function target(...) end

function rule(...) end

function option(...) end

-- lifecycle callbacks
function on_load(...) end

function on_config(...) end

function on_install(...) end

function on_package(...) end

function after_build(...) end

-- misc sandbox builtins
function import(...) end

function cprint(...) end

-- xmake-specific global modules (not part of stock Lua)
path = {}
utils = {}
