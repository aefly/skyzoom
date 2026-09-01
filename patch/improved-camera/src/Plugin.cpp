/*
 * SkyZoomICPatch - SkyZoom compatibility patch for Improved Camera SE.
 * Optional - only install this alongside SkyZoom.dll if Improved Camera SE
 * is also installed. See ../README.md for why it's needed.
 */

#include "Hook.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace {
// Documents\My Games\Skyrim Special Edition\SKSE\SkyZoomICPatch.log
void InitializeLogging() {
  auto path = SKSE::log::log_directory();
  if (!path) {
    return;
  }
  *path /= "SkyZoomICPatch.log"sv;

  std::shared_ptr<spdlog::logger> log;
  if (REX::W32::IsDebuggerPresent()) {
    log = std::make_shared<spdlog::logger>(
        "Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
  } else {
    log = std::make_shared<spdlog::logger>(
        "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                      path->string(), true));
  }

  log->set_level(spdlog::level::info);
  log->flush_on(spdlog::level::info);

  spdlog::set_default_logger(std::move(log));
  spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);
}
} // namespace

SKSEPluginLoad(const SKSE::LoadInterface *a_skse) {
  SKSE::Init(a_skse);
  InitializeLogging();

  if (auto *decl = SKSE::PluginDeclaration::GetSingleton()) {
    SKSE::log::info("SkyZoomICPatch v{}", decl->GetVersion().string("."));
  }
  SKSE::log::info("SkyZoomICPatch loaded");

  Hook::Install();

  return true;
}
