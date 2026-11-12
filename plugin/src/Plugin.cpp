#include "Core.h"
#include "WeaponClasses.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <toml++/toml.hpp>
namespace {
sp::Model model;
void message(SKSE::MessagingInterface::Message* msg) {
    if(msg->type!=SKSE::MessagingInterface::kDataLoaded)return;
    try {
        model.loadConfig("Data/SKSE/Plugins/SkillPractice");
        size_t count=0;for(auto& sk:model.skills){count+=sk.subs.size();spdlog::info("[boot] skill={} subskills={}",sk.av,sk.subs.size());for(auto& [g,b]:sk.budgets)for(auto& s:sk.subs)if(s.group==g&&!s.fixed){spdlog::info("[boot] skill={} group={} budget={} k={}",sk.av,g,b,s.k);break;}}
        spdlog::info("[boot] managed={} subskills={}",model.skills.size(),count);
        spdlog::debug("[cfg] {}",model.settings.values.dump());
        for(size_t i=0;i<sp::weaponClasses.size();++i){auto& row=sp::weaponClasses[i];auto kw=RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSKeyword>(static_cast<RE::FormID>(0x800+i),"SkillPractice.esp");if(kw)spdlog::info("[boot] keyword={} resolved={:08X}",row.keyword,kw->GetFormID());else spdlog::warn("[boot] missing keyword={} local={:X}",row.keyword,0x800+i);}
    }catch(const std::exception& e){spdlog::critical("[boot] config error: {}",e.what());}
}
}
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    auto path=SKSE::log::log_directory();if(!path)return false;
    auto sink=std::make_shared<spdlog::sinks::basic_file_sink_mt>((*path/"SkillPractice.log").string(),true);
    auto logger=std::make_shared<spdlog::logger>("SkillPractice",sink);spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");spdlog::flush_on(spdlog::level::warn);spdlog::flush_every(std::chrono::seconds(1));
    spdlog::info("[boot] SkillPractice 0.1.0 runtime={}",skse->RuntimeVersion().string());
    return SKSE::GetMessagingInterface()->RegisterListener(message);
}
