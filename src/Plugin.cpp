/*
 * SkyZoom - Simple hold-to-zoom feature for Skyrim SE/AE.
 * This plugin links against CommonLibSSE-NG.
 */

#include "Config.h"
#include "Hooks.h"
#include "Input.h"
#include "Papyrus.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace {
// RE::BSInputDeviceManager isn't guaranteed to exist yet at plugin load -
// wait for SKSE's kInputLoaded message before registering the wheel sink.
void OnSKSEMessage(SKSE::MessagingInterface::Message *a_message) {
  if (a_message->type == SKSE::MessagingInterface::kInputLoaded) {
    Input::InstallWheelSink();
    Input::InstallTriggerSink();
  }
}

// Documents\My Games\Skyrim Special Edition\SKSE\SkyZoom.log
void InitializeLogging() {
  auto path = SKSE::log::log_directory();
  if (!path) {
    return;
  }
  *path /= "SkyZoom.log"sv;

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
    SKSE::log::info("SkyZoom v{}", decl->GetVersion().string("."));
  }
  SKSE::log::info("SkyZoom loaded");

  Config::Load();
  Hooks::Install();
  Papyrus::Register();

  if (auto *messaging = SKSE::GetMessagingInterface()) {
    messaging->RegisterListener(OnSKSEMessage);
  }

  return true;
}
