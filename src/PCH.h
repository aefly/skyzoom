#pragma once

// Use REX::W32 instead of real <Windows.h> - though a few RE/* headers here
// (e.g. RendererShadowState.h) pull in the real <d3d11.h>/<windows.h>
// anyway, which shadows a handful of REX::W32:: names with object-like
// macros (MAX_PATH, VK_*, PAGE_READWRITE, ...). Where that bites, the
// affected .cpp does a local #undef right before using the REX::W32:: name.
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace std::literals;
