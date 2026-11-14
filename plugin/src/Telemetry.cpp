#include <MinHook.h>

#include <deque>
#include <mutex>

#include "Runtime.h"
namespace runtime {
std::uint64_t frame = 0;
DWORD mainThread = 0;
using Clock = std::chrono::steady_clock;
std::uint64_t stamp() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             Clock::now().time_since_epoch())
      .count();
}
namespace {
struct Context {
  RE::FormID form = 0;
  std::uint64_t time = 0;
  bool left = false, bash = false;
};
Context swing, hit, cast, craft, bashCtx;
bool loaded = false, telemetryOnly = true;
bool firstLoadedUpdate = false;
// A task submitted to SKSE can be dispatched on a different thread during load.
// Never call a thread-checking event sink recursively through that task queue.
std::mutex eventMutex;
std::deque<std::function<void()>> eventQueue;
void enqueueEvent(std::function<void()> event) {
  std::lock_guard lock(eventMutex);
  if (eventQueue.size() < 4096) {
    eventQueue.push_back(std::move(event));
    return;
  }
  static std::atomic<unsigned> dropped{0};
  if (dropped.fetch_add(1, std::memory_order_relaxed) % 1000 == 0)
    spdlog::warn("[boot] deferred event queue full; event dropped");
}
void drainEvents() {
  std::deque<std::function<void()>> batch;
  {
    std::lock_guard lock(eventMutex);
    batch.swap(eventQueue);
  }
  for (auto& event : batch) event();
}
double tickElapsed = 0;
using UseFn = void (*)(RE::Actor*, RE::ActorValue, float, RE::TESForm*);
REL::Relocation<UseFn> originalUse;
using UpdateFn = void (*)(RE::PlayerCharacter*, float);
REL::Relocation<UpdateFn> originalUpdate;
using AdvanceFn = void (*)(RE::PlayerCharacter*, RE::ActorValue, float,
                           RE::TESForm*, std::uint32_t);
AdvanceFn originalAdvance = nullptr;
void advance(RE::PlayerCharacter* p, RE::ActorValue value, float points,
             RE::TESForm* object, std::uint32_t kind) {
  spdlog::debug(
      "[use] path=AddSkillExperience t_us={} frame={} av={} pts={} arg={:08X} "
      "action={}",
      stamp(), frame, int(value), points, object ? object->GetFormID() : 0,
      kind);
  originalAdvance(p, value, points, object, kind);
}
std::string owner(std::uintptr_t address) {
  HMODULE module = nullptr;
  char path[MAX_PATH]{};
  if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(address), &module))
    GetModuleFileNameA(module, path, MAX_PATH);
  return path;
}
bool fresh(const Context& c) {
  return c.time && stamp() - c.time <
                       model.settings.get("xp", "context_window_ms") * 1000;
}
struct Pending {
  RE::ActorValue skill;
  float points;
  RE::FormID arg;
  std::uint64_t time;
};
std::deque<Pending> pending;
void context(const char* name, const Context& c) {
  spdlog::debug("[ctx] t_us={} frame={} event={} form={:08X} left={} bash={}",
                stamp(), frame, name, c.form, c.left, c.bash);
}
void useOnMain(RE::Actor*, RE::ActorValue, float, RE::TESForm*);
void use(RE::Actor* who, RE::ActorValue av, float points, RE::TESForm* arg) {
  spdlog::debug(
      "[use] t_us={} frame={} av={} pts={} arg={:08X} type={} name={} "
      "thread={}",
      stamp(), frame, int(av), points, arg ? arg->GetFormID() : 0,
      arg ? int(arg->GetFormType()) : 0, arg ? arg->GetName() : "",
      GetCurrentThreadId());
  if (who != RE::PlayerCharacter::GetSingleton() || telemetryOnly) {
    originalUse(who, av, points, arg);
    return;
  }
  if (GetCurrentThreadId() != mainThread) {
    // The model lives on the main thread. A managed award from a worker thread
    // is suppressed here and replayed by the next player update; passing it
    // through would leak vanilla XP (later seen as an external change).
    if (!managed(av)) {
      originalUse(who, av, points, arg);
      return;
    }
    spdlog::debug("[use] deferred from thread={}", GetCurrentThreadId());
    enqueueEvent([who, av, points, arg] { use(who, av, points, arg); });
    return;
  }
  guarded("UseSkill", [&] { useOnMain(who, av, points, arg); });
}
void useOnMain(RE::Actor* who, RE::ActorValue av, float points,
               RE::TESForm* arg) {
  auto sk = skill(av);
  if (!sk) {
    originalUse(who, av, points, arg);
    return;
  }
  if (!std::isfinite(points) || points <= 0 ||
      (!sk->initialized && model.settings.flag("debug", "fixed_xp"))) {
    if (!sk->initialized || model.settings.flag("debug", "vanilla_passthrough"))
      originalUse(who, av, points, arg);
    return;
  }
  if (!ready(*sk)) {
    originalUse(who, av, points, arg);
    return;
  }
  if (std::isfinite(points) && points > 0 &&
      !model.settings.flag("debug", "fixed_xp")) {
    if (arg)
      route(av, points, arg, "use", fresh(bashCtx));
    else
      pending.push_back({av, points, 0, stamp()});
  }
  if (model.settings.flag("debug", "vanilla_passthrough"))
    originalUse(who, av, points, arg);
}
void drain(bool force = false) {
  auto now = stamp();
  for (auto it = pending.begin(); it != pending.end();) {
    RE::TESForm* form = nullptr;
    bool bash = fresh(bashCtx);
    if (it->skill == RE::ActorValue::kSmithing && fresh(craft))
      form = RE::TESForm::LookupByID(craft.form);
    else if (it->skill == RE::ActorValue::kDestruction ||
             it->skill == RE::ActorValue::kRestoration) {
      if (fresh(cast)) {
        auto s = RE::TESForm::LookupByID<RE::SpellItem>(cast.form);
        if (s && classify(s).actorValue == it->skill) form = s;
      }
    } else if (fresh(hit))
      form = RE::TESForm::LookupByID(hit.form);
    if (form || force ||
        now - it->time >=
            model.settings.get("xp", "context_window_ms") * 1000) {
      route(it->skill, it->points, form, "use", bash);
      it = pending.erase(it);
    } else
      ++it;
  }
}
void updateOnMain(float, bool);
void update(RE::PlayerCharacter* player, float dt) {
  const bool probe =
      loaded && !firstLoadedUpdate && GetCurrentThreadId() == mainThread;
  if (probe) {
    firstLoadedUpdate = true;
    spdlog::info("[load] first player update: original begin");
    spdlog::default_logger()->flush();
  }
  originalUpdate(player, dt);
  if (probe) {
    spdlog::info("[load] first player update: original complete");
    spdlog::default_logger()->flush();
  }
  ++frame;
  if (GetCurrentThreadId() != mainThread) {
    static std::atomic<bool> warned{false};
    if (!warned.exchange(true))
      spdlog::warn(
          "[boot] player update on thread={} but main thread={}; this update "
          "is ignored",
          GetCurrentThreadId(), mainThread);
    return;
  }
  guarded("PlayerUpdate", [&] { updateOnMain(dt, probe); });
}
void updateOnMain(float dt, bool probe) {
  drainEvents();
  if (!loaded) return;
  if (!telemetryOnly) drain();
  auto ui = RE::UI::GetSingleton();
  bool paused = !ui || ui->GameIsPaused() ||
                ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
                ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME);
  static auto lastHeartbeat = Clock::now();
  if (Clock::now() - lastHeartbeat >= std::chrono::seconds(15)) {
    lastHeartbeat = Clock::now();
    spdlog::info(
        "[health] player update frame={} thread={} paused={} loadingMenu={} "
        "mainMenu={} dt={}",
        frame, GetCurrentThreadId(), paused,
        ui && ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME),
        ui && ui->IsMenuOpen(RE::MainMenu::MENU_NAME), dt);
  }
  double hours = RE::Calendar::GetSingleton()->GetHoursPassed();
  tickElapsed += std::max(0.f, dt);
  if (paused) {
    if (hours - model.lastHours >
        model.settings.get("decay", "max_tick_game_hours"))
      spdlog::debug("[time] skipped paused jump={} hours={}",
                    hours - model.lastHours, hours);
    model.lastHours = hours;
    tickElapsed = 0;
  } else if (tickElapsed >= model.settings.get("decay", "tick_seconds")) {
    double delta = hours - model.lastHours;
    model.lastHours = hours;
    tickElapsed = 0;
    if (delta > 0 &&
        delta <= model.settings.get("decay", "max_tick_game_hours")) {
      if (!telemetryOnly) {
        std::vector<double> before;
        for (auto& sk : model.skills)
          for (auto& sub : sk.subs)
            before.push_back(sub.current.value(sub.curve));
        model.tick(delta);
        size_t i = 0;
        for (auto& sk : model.skills)
          for (auto& sub : sk.subs) {
            double after = sub.current.value(sub.curve);
            if (int(before[i]) != int(after))
              spdlog::debug("[decay] sub={} cur={}->{} floor={} U={}", sub.id,
                            before[i], after, sub.floor.value(sub.curve),
                            sub.idle);
            ++i;
          }
        spdlog::trace("[decay] tick dt={} scale={} activeHours={}", delta,
                      model.settings.get("debug", "time_scale"),
                      model.activeHours);
        synchronize();
      }
    } else if (delta != 0)
      spdlog::debug("[time] skipped delta={} hours={}", delta, hours);
  }
  if (probe) {
    spdlog::info(
        "[load] first player update: model complete; aim/UI begin paused={}",
        paused);
    spdlog::default_logger()->flush();
  }
  if (!telemetryOnly) refreshUI(paused ? 0 : dt);
  if (probe) {
    spdlog::info("[load] first player update: complete");
    spdlog::default_logger()->flush();
  }
}
class Events : public RE::BSTEventSink<RE::TESHitEvent>,
               public RE::BSTEventSink<RE::TESSpellCastEvent>,
               public RE::BSTEventSink<RE::ItemCrafted::Event>,
               public RE::BSTEventSink<RE::BSAnimationGraphEvent>,
               public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
  RE::BSEventNotifyControl ProcessEvent(
      const RE::TESHitEvent* e, RE::BSTEventSource<RE::TESHitEvent>*) override {
    if (e && GetCurrentThreadId() != mainThread) {
      auto copy = *e;
      enqueueEvent([this, copy] { ProcessEvent(&copy, nullptr); });
      return RE::BSEventNotifyControl::kContinue;
    }
    guarded("event", [&] {
      if (!e) return;
      auto p = RE::PlayerCharacter::GetSingleton();
      bool isBash = e->flags.any(RE::TESHitEvent::Flag::kBashAttack);
      if (e->cause.get() == p) {
        hit = {e->source, stamp(), false, isBash};
        context("hit", hit);
        if (isBash) bashCtx = hit;
        if (!telemetryOnly && model.settings.flag("debug", "fixed_xp")) {
          auto f = RE::TESForm::LookupByID(e->source);
          auto w = f ? f->As<RE::TESObjectWEAP>() : nullptr;
          if (isBash)
            route(RE::ActorValue::kBlock, 0, f, "fixed", true);
          else if (!f || w) {
            auto sub = weapon(w);
            if (sub.starts_with("1h-"))
              route(RE::ActorValue::kOneHanded, 0, f, "fixed");
            else if (sub.starts_with("2h-"))
              route(RE::ActorValue::kTwoHanded, 0, f, "fixed");
          }
        }
      }
      if (e->target.get() == p &&
          e->flags.any(RE::TESHitEvent::Flag::kHitBlocked)) {
        context("blocked", {e->source, stamp()});
        if (!telemetryOnly && model.settings.flag("debug", "fixed_xp"))
          route(RE::ActorValue::kBlock, 0, nullptr, "fixed");
      }
      if (!telemetryOnly) drain();
    });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(
      const RE::TESSpellCastEvent* e,
      RE::BSTEventSource<RE::TESSpellCastEvent>*) override {
    if (e && GetCurrentThreadId() != mainThread) {
      auto copy = *e;
      enqueueEvent([this, copy] { ProcessEvent(&copy, nullptr); });
      return RE::BSEventNotifyControl::kContinue;
    }
    guarded("event", [&] {
      if (e && e->object.get() == RE::PlayerCharacter::GetSingleton()) {
        cast = {e->spell, stamp()};
        context("cast", cast);
        auto f = RE::TESForm::LookupByID<RE::SpellItem>(e->spell);
        if (!telemetryOnly && f) {
          auto c = classify(f);
          if (model.settings.flag("debug", "fixed_xp"))
            route(c.actorValue, 0, f, "fixed");
          drain();
        }
      }
    });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(
      const RE::ItemCrafted::Event* e,
      RE::BSTEventSource<RE::ItemCrafted::Event>*) override {
    if (e && GetCurrentThreadId() != mainThread) {
      auto copy = *e;
      enqueueEvent([this, copy] { ProcessEvent(&copy, nullptr); });
      return RE::BSEventNotifyControl::kContinue;
    }
    guarded("event", [&] {
      if (e && e->item) {
        craft = {e->item->GetFormID(), stamp()};
        context("craft", craft);
        auto ui = RE::UI::GetSingleton();
        if (!telemetryOnly && ui && ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME)) {
          if (model.settings.flag("debug", "fixed_xp"))
            route(RE::ActorValue::kSmithing, 0, e->item, "fixed");
          drain();
        }
      }
    });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(
      const RE::BSAnimationGraphEvent* e,
      RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override {
    if (e && GetCurrentThreadId() != mainThread) {
      auto copy = *e;
      enqueueEvent([this, copy] { ProcessEvent(&copy, nullptr); });
      return RE::BSEventNotifyControl::kContinue;
    }
    guarded("event", [&] {
      if (e && e->holder == RE::PlayerCharacter::GetSingleton()) {
        std::string tag = e->tag.c_str();
        if (tag == "weaponSwing" || tag == "weaponLeftSwing") {
          swing = {0, stamp(), tag == "weaponLeftSwing"};
          auto item =
              RE::PlayerCharacter::GetSingleton()->GetEquippedObject(swing.left);
          swing.form = item ? item->GetFormID() : 0;
          context(tag.c_str(), swing);
        } else if (tag == "bashRelease" || tag == "bashPowerStart") {
          bashCtx = {0, stamp(), false, true};
          context(tag.c_str(), bashCtx);
        }
      }
    });
    return RE::BSEventNotifyControl::kContinue;
  }
  RE::BSEventNotifyControl ProcessEvent(
      const RE::MenuOpenCloseEvent* e,
      RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
    if (e && GetCurrentThreadId() != mainThread) {
      auto copy = *e;
      enqueueEvent([this, copy] { ProcessEvent(&copy, nullptr); });
      return RE::BSEventNotifyControl::kContinue;
    }
    guarded("event", [&] {
      if (e) {
        spdlog::debug("[ctx] t_us={} frame={} menu={} open={}", stamp(), frame,
                      e->menuName.c_str(), e->opening);
        if (e->menuName == RE::RaceSexMenu::MENU_NAME && !e->opening)
          model.raceClosed = true;
        if (e->menuName == RE::JournalMenu::MENU_NAME && !e->opening &&
            !telemetryOnly)
          reloadINI();  // MCM lives in the journal; TOML only on reloadConfig
      }
    });
    return RE::BSEventNotifyControl::kContinue;
  }
};
Events events;
}  // namespace
RE::TESForm* swingWeapon() {
  return fresh(swing) ? RE::TESForm::LookupByID(swing.form) : nullptr;
}
void clearDeferredEvents() {
  std::lock_guard lock(eventMutex);
  eventQueue.clear();
}
RE::SpellItem* recentSpell(RE::ActorValue value) {
  if (!fresh(cast)) return nullptr;
  auto s = RE::TESForm::LookupByID<RE::SpellItem>(cast.form);
  return s && classify(s).actorValue == value ? s : nullptr;
}
void resetContext() {
  {
    std::lock_guard lock(eventMutex);
    eventQueue.clear();
  }
  pending.clear();
  swing = hit = cast = craft = bashCtx = {};
  tickElapsed = 0;
  firstLoadedUpdate = false;
  loaded = true;
  model.lastHours = RE::Calendar::GetSingleton()->GetHoursPassed();
  auto p = RE::PlayerCharacter::GetSingleton();
  if (p) p->AddAnimationGraphEventSink(&events);
}
void installTelemetry() {
  mainThread = GetCurrentThreadId();
  REL::Relocation<std::uintptr_t> vt{RE::VTABLE_PlayerCharacter[0]};
  originalUse = vt.write_vfunc(0xF7, use);
  originalUpdate = vt.write_vfunc(0xAD, update);
  spdlog::info(
      "[boot] hook=PlayerCharacter::UseSkill vslot=F7 originalModule={}",
      owner(originalUse.address()));
  spdlog::info(
      "[boot] hook=PlayerCharacter::Update vslot=AD originalModule={} "
      "mainThread={}",
      owner(originalUpdate.address()), mainThread);
  auto src = RE::ScriptEventSourceHolder::GetSingleton();
  src->AddEventSink<RE::TESHitEvent>(&events);
  src->AddEventSink<RE::TESSpellCastEvent>(&events);
  RE::ItemCrafted::GetEventSource()->AddEventSink(&events);
  RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(&events);
  auto address =
      REL::Relocation<std::uintptr_t>{REL::RelocationID(39413, 40488)}
          .address();
  auto status = MH_Initialize();
  if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED) {
    status = MH_CreateHook(reinterpret_cast<void*>(address),
                           reinterpret_cast<void*>(advance),
                           reinterpret_cast<void**>(&originalAdvance));
    if (status == MH_OK)
      status = MH_EnableHook(reinterpret_cast<void*>(address));
  }
  if (status == MH_OK)
    spdlog::info(
        "[boot] hook=AddSkillExperience AddressLibrary=39413/40488 "
        "originalModule={} logOnly=true",
        owner(reinterpret_cast<std::uintptr_t>(originalAdvance)));
  else
    spdlog::error("[boot] AddSkillExperience telemetry failed={}", int(status));
}
void installGameplay() {
  telemetryOnly = false;
  spdlog::info(
      "[boot] gameplay routing enabled; fixed XP tempering is not covered");
}
}  // namespace runtime
