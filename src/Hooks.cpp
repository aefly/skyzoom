#include "Hooks.h"

#include "FOVController.h"

// Some RE/* headers pull in the real <d3d11.h> (and therefore <windows.h>)
// after REX/W32/BASE.h has already been parsed, so its "no real Windows.h"
// guard doesn't catch it. That leaves PAGE_READWRITE as an object-like
// macro, shadowing REX::W32::PAGE_READWRITE - undef it first.
#undef PAGE_READWRITE

namespace Hooks {
namespace {
using Present_t = std::int32_t (*)(REX::W32::IDXGISwapChain *, std::uint32_t,
                                   std::uint32_t);

Present_t g_originalPresent = nullptr;
std::atomic<bool> g_presentHooked{false};

// Present is vtable slot 8 (IUnknown x3 + IDXGIObject x4 + IDXGIDeviceSubObject
// x1).
constexpr std::size_t kPresentVTableIndex = 8;

template <typename T>
void PatchVTableSlot(void *a_object, std::size_t a_index, void *a_hook,
                     T &a_outOriginal) {
  auto *slot = *reinterpret_cast<void ***>(a_object) + a_index;

  std::uint32_t oldProtect = 0;
  REX::W32::VirtualProtect(slot, sizeof(void *), REX::W32::PAGE_READWRITE,
                           &oldProtect);
  a_outOriginal = reinterpret_cast<T>(slot[0]);
  slot[0] = a_hook;
  REX::W32::VirtualProtect(slot, sizeof(void *), oldProtect, &oldProtect);
}

std::int32_t hkPresent(REX::W32::IDXGISwapChain *a_swapChain,
                       std::uint32_t a_syncInterval, std::uint32_t a_flags) {
  FOVController::Update();

  return g_originalPresent(a_swapChain, a_syncInterval, a_flags);
}

REX::W32::IDXGISwapChain *GetGameSwapChain() {
  auto *renderer = RE::BSGraphics::Renderer::GetSingleton();
  return renderer ? renderer->GetRuntimeData().renderWindows[0].swapChain
                  : nullptr;
}

// No timeout: a heavily modded load order can take a while to reach the
// renderer, and polling is nearly free.
std::uint32_t ThreadProc(void *) noexcept {
  for (int i = 0;; ++i) {
    if (auto *swapChain = GetGameSwapChain()) {
      bool expected = false;
      if (g_presentHooked.compare_exchange_strong(expected, true)) {
        PatchVTableSlot(swapChain, kPresentVTableIndex,
                        reinterpret_cast<void *>(&hkPresent),
                        g_originalPresent);
        SKSE::log::info("SkyZoom: real swapchain found ({:X}), Present hooked",
                        reinterpret_cast<std::uintptr_t>(swapChain));
      }
      return 0;
    }

    if (i > 0 && i % 300 == 0) { // every ~30s
      SKSE::log::warn("SkyZoom: still waiting for the renderer's swapchain "
                      "({}s elapsed)",
                      i / 10);
    }
    REX::W32::Sleep(100);
  }
}
} // namespace

void Install() {
  // Detached thread; close the handle immediately rather than leak it.
  auto *thread =
      REX::W32::CreateThread(nullptr, 0, &ThreadProc, nullptr, 0, nullptr);
  if (thread) {
    REX::W32::CloseHandle(thread);
  }
}
} // namespace Hooks
