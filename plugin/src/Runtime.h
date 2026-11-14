#pragma once
#include <atomic>

#include "Core.h"
namespace runtime {
// Game hooks must never let a C++ exception escape into engine code.
template <class F>
void guarded(const char* where, F&& f) noexcept {
  try {
    f();
  } catch (const std::exception& e) {
    static std::atomic<unsigned> count{0};
    if (count.fetch_add(1, std::memory_order_relaxed) < 50)
      spdlog::error("[boot] exception in {}: {}", where, e.what());
  }
}
extern sp::Model model;
void start();
void message(SKSE::MessagingInterface::Message*);
void installTelemetry();
void installGameplay();
void clearDeferredEvents();
void installAim();
void setupUI();
void refreshUI(float dt);
void closeUI();
void reload();
void reloadINI();
// Managed actor values as a bit mask; safe to read from any thread.
bool managed(RE::ActorValue);
void synchronize();
void initialize(bool force = false);
void action(const char*);
sp::json snapshot();
std::uint64_t stamp();
extern std::uint64_t frame;
extern DWORD mainThread;
RE::ActorValue av(std::string_view);
sp::Skill* skill(RE::ActorValue);
bool ready(sp::Skill&);
void award(std::string_view, double, std::string_view, double points = -1);
struct Classification {
  std::string school, form, direction, projectile;
  RE::ActorValue actorValue = RE::ActorValue::kNone;
};
Classification classify(RE::SpellItem*);
std::string weapon(RE::TESObjectWEAP*);
std::string weaponKeyword(RE::TESObjectWEAP*);
double beamAmplitude();
void resolveForms();
void dumpWeapons();
void dumpSpells();
void route(RE::ActorValue, float, RE::TESForm*, std::string_view,
           bool bash = false);
}  // namespace runtime
