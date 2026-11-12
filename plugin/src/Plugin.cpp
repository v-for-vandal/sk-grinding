#include <spdlog/sinks/basic_file_sink.h>

#include "Runtime.h"
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
  SKSE::Init(skse);
  auto path = SKSE::log::log_directory();
  if (!path) return false;
  auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      (*path / "SkillPractice.log").string(), true);
  auto logger = std::make_shared<spdlog::logger>("SkillPractice", sink);
  spdlog::set_default_logger(logger);
  spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
  spdlog::flush_on(spdlog::level::warn);
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::info("[boot] SkillPractice 0.1.3 runtime={}",
               skse->RuntimeVersion().string());
  if (skse->RuntimeVersion() != REL::Version{1, 6, 1170, 0}) {
    spdlog::critical("[boot] prototype supports only runtime 1.6.1170");
    return false;
  }
  runtime::start();
  return SKSE::GetMessagingInterface()->RegisterListener(runtime::message);
}
