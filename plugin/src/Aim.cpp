#include <atomic>
#include <memory>
#include <mutex>
#include <numbers>
#include <random>
#include <unordered_map>

#include "Runtime.h"
#include "RuntimeMath.h"
namespace runtime {
namespace {
std::mt19937 rng{std::random_device{}()};
double phaseX = 0, phaseZ = 0, lastX = 0, lastZ = 0, held = 0;
double amplitudeDegrees = 0;
bool swaying = false;
struct Noise {
  double sigma = 0, fixed = 0;
};
using NoiseMap = std::unordered_map<RE::FormID, Noise>;
// Projectile callbacks run on workers in this LoreRim setup. Publish only
// immutable aim parameters from the main thread, rather than reading skill
// levels/config concurrently with XP awards, loading or UI debug actions.
std::atomic<std::shared_ptr<const NoiseMap>> missileNoise{
    std::make_shared<const NoiseMap>()};
std::mutex seenMutex;
std::unordered_map<std::uint32_t, std::uint64_t> seen;
using GetVelocity = void (*)(RE::Projectile*, RE::NiPoint3&);
REL::Relocation<GetVelocity> missileVelocity, grenadeVelocity;

void cacheMissileNoise(RE::PlayerCharacter* player) {
  auto next = std::make_shared<NoiseMap>();
  if (model.raceClosed && model.settings.flag("aim", "missile_enabled")) {
    auto add = [&](RE::SpellItem* spell) {
      if (!spell || next->contains(spell->GetFormID())) return;
      auto c = classify(spell);
      if (c.form != "missile") return;
      auto sub = model.find("form/missile");  // shared by all schools
      if (!sub || !skill(c.actorValue)) return;
      // Before first practice, use the same provisional start level as init.
      // Requiring initialized=true made the first cast perfectly accurate even
      // though initialization itself waits for the spell's first XP event.
      const double level = model.shared.initialized
                               ? sub->current.value(sub->curve)
                               : model.startLevel(bestSchool());
      (*next)[spell->GetFormID()] = {
          model.settings.get("aim", "missile_sigma_max_deg") *
              std::max(0., 1 - level / model.settings.get("aim", "missile_L0")),
          model.settings.get("aim", "debug_missile_yaw_deg")};
    };
    for (bool left : {false, true}) {
      auto item = player->GetEquippedObject(left);
      add(item ? item->As<RE::SpellItem>() : nullptr);
    }
    for (auto hand : {RE::MagicSystem::CastingSource::kLeftHand,
                      RE::MagicSystem::CastingSource::kRightHand,
                      RE::MagicSystem::CastingSource::kOther}) {
      auto caster = player->GetMagicCaster(hand);
      add(caster && caster->currentSpell
              ? caster->currentSpell->As<RE::SpellItem>()
              : nullptr);
    }
  }
  missileNoise.store(std::move(next));
}

void perturb(RE::Projectile* projectile, RE::NiPoint3& output,
             const RE::NiPoint3& before, std::uint32_t targetBefore) {
  auto& data = projectile->GetProjectileRuntimeData();
  // TDM waits for the 3D node before its initial prediction. Do not consume
  // our one-shot perturbation before that same prerequisite is satisfied.
  if (!projectile->Get3D2() || data.livingTime > 0.1f || !data.spell ||
      data.shooter.get().get() != RE::PlayerCharacter::GetSingleton())
    return;
  auto cache = missileNoise.load();
  auto it = cache->find(data.spell->GetFormID());
  if (it == cache->end()) return;
  const auto [sigma, fixed] = it->second;
  if (sigma <= 0 && fixed == 0) return;
  const auto v = data.linearVelocity;
  if (v.x * v.x + v.y * v.y + v.z * v.z < 1e-6f) return;
  const auto handle = projectile->GetHandle().native_handle();
  if (!handle) return;
  const auto now = stamp();
  {
    std::lock_guard lock(seenMutex);
    if (seen.contains(handle)) return;
    // The getter can be called repeatedly for one projectile. Perturb once,
    // not once per query (which would cause a random walk during flight).
    // Handles include a generation, unlike recycled dynamic FormIDs. Entries
    // expire after 5 s; the 0.1 s eligibility guard prevents a live old missile
    // receiving another offset after its entry expires.
    std::erase_if(seen, [now](const auto& entry) {
      return now - entry.second > 5000000;
    });
    seen.emplace(handle, now);
  }
  thread_local std::mt19937 projectileRng{std::random_device{}()};
  std::normal_distribution<double> distribution(
      0, std::max(sigma, 1e-9) * std::numbers::pi / 180);
  const double pitch = fixed != 0 ? 0 : distribution(projectileRng);
  const double yaw =
      fixed != 0 ? fixed * std::numbers::pi / 180 : distribution(projectileRng);
  auto rotated = sp::rotateVelocity(v.x, v.y, v.z, pitch, yaw);
  data.linearVelocity =
      RE::NiPoint3{float(rotated[0]), float(rotated[1]), float(rotated[2])};
  // Preserve desiredTarget: Predictive uses it as its "already aimed" marker.
  // Clearing it would let TDM aim again and erase this offset on a later call.
  // Disable engine aim-assist flags without deleting TDM's completion marker.
  data.flags.reset(RE::Projectile::Flags::kAutoAim,
                   RE::Projectile::Flags::kAlwaysHit);
  projectile->data.angle.x =
      float(-std::atan2(rotated[2], std::hypot(rotated[0], rotated[1])));
  projectile->data.angle.z = float(std::atan2(rotated[0], rotated[1]));
  // The preceding getter may have copied its result before TDM changed the
  // projectile's velocity. Keep the returned value and runtime velocity equal.
  output = data.linearVelocity;
  spdlog::info(
      "[aim] post-velocity spell={:08X} projectile={:08X} sigma={} "
      "pitch={} yaw={} targetBefore={:08X} targetAfter={:08X} "
      "before=({},{},{}) aimed=({},{},{}) final=({},{},{}) thread={}",
      data.spell->GetFormID(), projectile->GetFormID(), sigma, pitch, yaw,
      targetBefore, data.desiredTarget.native_handle(), before.x, before.y,
      before.z, v.x, v.y, v.z, output.x, output.y, output.z,
      GetCurrentThreadId());
}
void getMissileVelocity(RE::Projectile* p, RE::NiPoint3& output) {
  const auto before = p->GetProjectileRuntimeData().linearVelocity;
  const auto target =
      p->GetProjectileRuntimeData().desiredTarget.native_handle();
  // In 0.1.4 we perturbed before UpdateImpl. Lock-on Predictive subsequently
  // overwrote that velocity. TDM's published GetLinearVelocity hook performs
  // prediction after its own predecessor; chain it first, then add our error.
  // Target/velocity diagnostics verify this order against the installed DLL.
  missileVelocity(p, output);
  guarded("projectile velocity", [&] { perturb(p, output, before, target); });
}
void getGrenadeVelocity(RE::Projectile* p, RE::NiPoint3& output) {
  // Lobbers share the one-shot noise rule and must also preserve target state.
  const auto before = p->GetProjectileRuntimeData().linearVelocity;
  const auto target =
      p->GetProjectileRuntimeData().desiredTarget.native_handle();
  grenadeVelocity(p, output);
  guarded("projectile velocity", [&] { perturb(p, output, before, target); });
}
}  // namespace
void resetAim() {
  missileNoise.store(std::make_shared<const NoiseMap>());
  std::lock_guard lock(seenMutex);
  seen.clear();
}
void installAim() {
  // The initial Projectile::Launch probe missed Firebolt in the first test;
  // later logs showed worker launches too. The 0.1.4 UpdateImpl fallback made
  // spread visible in Free Aim but ran before TDM Predictive's correction.
  // Hook the velocity getter instead; leave unrelated TDM camera/lock-on
  // features intact. Continuous Homing can still cancel a one-shot offset.
  REL::Relocation<std::uintptr_t> missile{RE::VTABLE_MissileProjectile[0]};
  REL::Relocation<std::uintptr_t> grenade{RE::VTABLE_GrenadeProjectile[0]};
  missileVelocity = missile.write_vfunc(0x86, getMissileVelocity);
  grenadeVelocity = grenade.write_vfunc(0x86, getGrenadeVelocity);
  auto owner = [](std::uintptr_t address) {
    HMODULE module = nullptr;
    char path[MAX_PATH]{};
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(address), &module))
      GetModuleFileNameA(module, path, MAX_PATH);
    return std::string(path);
  };
  spdlog::info(
      "[boot] aim=post-velocity MissileProjectile/GrenadeProjectile "
      "vslot=86 missileOriginal={:X} grenadeOriginal={:X}",
      missileVelocity.address(), grenadeVelocity.address());
  spdlog::info("[boot] aim velocity chain missileModule={} grenadeModule={}",
               owner(missileVelocity.address()),
               owner(grenadeVelocity.address()));
}
void refreshAim(float dt) {
  auto p = RE::PlayerCharacter::GetSingleton();
  if (!p) return;
  cacheMissileNoise(p);
  if (dt <= 0) return;
  double level = 1e9;
  bool casting = false;
  if (model.settings.flag("aim", "beam_enabled"))
    for (auto hand : {RE::MagicSystem::CastingSource::kLeftHand,
                      RE::MagicSystem::CastingSource::kRightHand,
                      RE::MagicSystem::CastingSource::kOther}) {
      auto caster = p->GetMagicCaster(hand);
      if (!caster || !caster->currentSpell ||
          !caster->state.any(RE::MagicCaster::State::kCharging,
                             RE::MagicCaster::State::kCasting))
        continue;
      auto spell = caster->currentSpell->As<RE::SpellItem>();
      auto c = classify(spell);
      if (c.form != "beam") continue;
      auto sub = model.find("form/beam");  // shared by all schools
      if (sub && skill(c.actorValue) && model.shared.initialized) {
        casting = true;
        level = std::min(level, sub->current.value(sub->curve));
      }
    }
  if (!casting) {
    if (swaying) spdlog::debug("[aim] sway stop held={}", held);
    swaying = false;
    held = lastX = lastZ = amplitudeDegrees = 0;
    return;
  }
  if (!swaying) {
    std::uniform_real_distribution<double> phase(0, 2 * std::numbers::pi);
    phaseX = phase(rng);
    phaseZ = phase(rng);
    held = lastX = lastZ = 0;
    swaying = true;
    spdlog::debug("[aim] sway start level={}", level);
  }
  held += dt;
  double amp =
      model.settings.get("aim", "beam_amp_max_deg") *
      std::max(0., 1 - level / model.settings.get("aim", "beam_L0")) *
      std::min(1., held / model.settings.get("aim", "beam_t_full_sec")) *
      std::numbers::pi / 180;
  amplitudeDegrees = amp * 180 / std::numbers::pi;
  double t = held * 2 * std::numbers::pi *
             model.settings.get("aim", "beam_noise_hz"),
         weight = model.settings.get("aim", "beam_secondary_weight");
  double x = amp *
             (std::sin(t + phaseX) +
              weight *
                  std::sin(t * model.settings.get(
                                   "aim", "beam_pitch_secondary_frequency") +
                           phaseZ)) /
             (1 + weight),
         z = amp *
             (std::sin(
                  t * model.settings.get("aim", "beam_yaw_primary_frequency") +
                  phaseZ) +
              weight * std::sin(t * model.settings.get(
                                        "aim", "beam_yaw_secondary_frequency") +
                                phaseX)) /
             (1 + weight);
  p->SetAngle(RE::NiPoint3{p->GetAngleX() + float(x - lastX), p->GetAngleY(),
                           p->GetAngleZ() + float(z - lastZ)});
  lastX = x;
  lastZ = z;
}
double beamAmplitude() { return amplitudeDegrees; }
}  // namespace runtime
