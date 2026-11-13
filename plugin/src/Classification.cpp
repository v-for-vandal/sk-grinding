#include <unordered_map>
#include <unordered_set>

#include "Runtime.h"
#include "SpellRules.h"
#include "WeaponClasses.h"
namespace runtime {
namespace {
std::array<RE::BGSKeyword*, 22> keywords{};
std::unordered_map<RE::FormID, std::string> weapons;
std::unordered_map<std::string, RE::BGSKeyword*> rules;
RE::TESRace *werewolf = nullptr, *vampireLord = nullptr;
RE::BGSKeyword *smelter = nullptr, *tanning = nullptr;
RE::BGSKeyword* keyword(const std::string& key) {
  auto it = rules.find(key);
  return it == rules.end() ? nullptr : it->second;
}
bool has(RE::TESForm* item, RE::BGSKeyword* kw) {
  if (!item || !kw) return false;
  if (auto w = item->As<RE::TESObjectWEAP>()) return w->HasKeyword(kw);
  if (auto a = item->As<RE::TESObjectARMO>()) return a->HasKeyword(kw);
  if (auto e = item->As<RE::EffectSetting>()) return e->HasKeyword(kw);
  return false;
}
std::string school(RE::ActorValue av) {
  if (av == RE::ActorValue::kDestruction) return "destruction";
  if (av == RE::ActorValue::kRestoration) return "restoration";
  return "";
}
}  // namespace
void resolveForms() {
  weapons.clear();
  rules.clear();
  auto dh = RE::TESDataHandler::GetSingleton();
  werewolf = dh->LookupForm<RE::TESRace>(0xCDD84, "Skyrim.esm");
  vampireLord = dh->LookupForm<RE::TESRace>(0x283A, "Dawnguard.esm");
  smelter = dh->LookupForm<RE::BGSKeyword>(0xA5CCE, "Skyrim.esm");
  tanning = dh->LookupForm<RE::BGSKeyword>(0x7866A, "Skyrim.esm");
  for (size_t i = 0; i < keywords.size(); ++i) {
    keywords[i] = dh->LookupForm<RE::BGSKeyword>(
        static_cast<RE::FormID>(0x800 + i), "SkillPractice.esp");
    if (keywords[i])
      spdlog::info("[boot] keyword={} resolved={:08X}",
                   sp::weaponClasses[i].keyword, keywords[i]->GetFormID());
    else
      spdlog::warn("[boot] missing keyword={}", sp::weaponClasses[i].keyword);
  }
  auto resolve = [&](const std::string& key) {
    if (rules.contains(key)) return;
    auto split = key.find('|');
    RE::BGSKeyword* kw = nullptr;
    try {
      if (split != std::string::npos)
        kw = dh->LookupForm<RE::BGSKeyword>(
            static_cast<RE::FormID>(
                std::stoul(key.substr(split + 1), nullptr, 0)),
            key.substr(0, split));
    } catch (...) {
    }
    rules[key] = kw;
    if (kw)
      spdlog::info("[boot] rule={} resolved={:08X}", key, kw->GetFormID());
    else
      spdlog::warn("[boot] missing rule={} skipped", key);
  };
  for (auto& row : model.settings.values["materials"]["material"])
    resolve(row.at("keyword"));
  for (auto school : {"destruction", "restoration"})
    for (auto& row : model.settings.values["directions"][school])
      for (auto& key : row["keywords"]) resolve(key);
}
std::string weapon(RE::TESObjectWEAP* w) {
  if (!w) return "1h-unarmed";
  if (auto it = weapons.find(w->GetFormID()); it != weapons.end())
    return it->second;
  std::string sub;
  unsigned count = 0;
  for (size_t i = 0; i < keywords.size(); ++i)
    if (keywords[i] && w->HasKeyword(keywords[i])) {
      if (!count) sub = sp::weaponClasses[i].sub;
      ++count;
    }
  if (count > 1)
    spdlog::warn("[class] multiple keywords form={:08X} name={} count={}",
                 w->GetFormID(), w->GetName(), count);
  if (!count) {
    auto t = w->GetWeaponType();
    switch (t) {
      case RE::WEAPON_TYPE::kBow:
      case RE::WEAPON_TYPE::kCrossbow:
        sub = "archery-other";
        break;
      case RE::WEAPON_TYPE::kStaff:
        sub = "";
        break;
      case RE::WEAPON_TYPE::kTwoHandSword:
      case RE::WEAPON_TYPE::kTwoHandAxe:
        sub = "2h-other";
        break;
      default:
        sub = "1h-other";
        break;
    }
    spdlog::warn("[class] fallback form={:08X} name={} sub={}", w->GetFormID(),
                 w->GetName(), sub);
  }
  weapons[w->GetFormID()] = sub;
  return sub;
}
std::string weaponKeyword(RE::TESObjectWEAP* w) {
  if (!w) return "unarmed";
  for (size_t i = 0; i < keywords.size(); ++i)
    if (keywords[i] && w->HasKeyword(keywords[i]))
      return std::string(sp::weaponClasses[i].keyword);
  return "fallback";
}
Classification classify(RE::SpellItem* spell) {
  Classification c;
  if (!spell) return c;
  auto e = spell->GetCostliestEffectItem();
  auto m = e ? e->baseEffect : nullptr;
  if (!m) return c;
  c.actorValue = m->data.associatedSkill;
  c.school = school(c.actorValue);
  if (c.school.empty()) return c;
  auto& d = m->data;
  auto p = d.projectileBase;
  auto delivery = spell->GetDelivery();
  auto casting = spell->GetCastingType();
  if (p)
    c.projectile = p->IsBeam()           ? "beam"
                   : p->IsGrenade()      ? "lobber"
                   : p->IsMissile()      ? "missile"
                   : p->IsArrow()        ? "arrow"
                   : p->IsFlamethrower() ? "flamethrower"
                   : p->IsCone()         ? "cone"
                   : p->IsBarrier()      ? "barrier"
                                         : "unknown";
  using D = RE::MagicSystem::Delivery;
  using A = RE::EffectSetting::Archetype;
  sp::SpellShape shape;
  using SD = sp::SpellShape::Delivery;
  shape.delivery = delivery == D::kSelf             ? SD::Self
                   : delivery == D::kTouch          ? SD::Touch
                   : delivery == D::kTargetActor    ? SD::Actor
                   : delivery == D::kTargetLocation ? SD::Location
                                                    : SD::Other;
  shape.summon = d.archetype == A::kSummonCreature;
  shape.cloak = d.archetype == A::kCloak;
  shape.concentration = casting == RE::MagicSystem::CastingType::kConcentration;
  shape.beam = p && p->IsBeam();
  shape.flame = p && (p->IsFlamethrower() || p->IsCone());
  shape.barrier = p && p->IsBarrier();
  shape.missile = p && (p->IsMissile() || p->IsGrenade() || p->IsArrow());
  shape.explosion = d.explosion || (p && p->data.explosionType);
  c.form = sp::spellForm(shape);
  c.direction = c.school + "/other";
  for (auto& row : model.settings.values["directions"][c.school]) {
    bool match = false;
    for (auto& key : row["keywords"])
      if (has(m, keyword(key))) match = true;
    if (match) {
      c.direction = row["subskill"];
      break;
    }
  }
  return c;
}
void dumpWeapons() {
  std::map<std::string, unsigned> counts;
  for (auto w :
       RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectWEAP>())
    if (w) {
      auto sub = weapon(w);
      ++counts[sub];
      spdlog::info("[class] form={:08X} plugin={} name={} sub={}",
                   w->GetFormID(), w->GetFile(0) ? w->GetFile(0)->fileName : "",
                   w->GetName(), sub);
    }
  for (auto& [sub, count] : counts)
    spdlog::info("[class] summary sub={} count={}", sub, count);
  spdlog::default_logger()->flush();
}
void dumpSpells() {
  auto p = RE::PlayerCharacter::GetSingleton();
  for (auto s :
       RE::TESDataHandler::GetSingleton()->GetFormArray<RE::SpellItem>())
    if (s && p->HasSpell(s)) {
      auto c = classify(s);
      spdlog::info(
          "[spell] form={:08X} name={} school={} casting={} delivery={} "
          "projectile={} formClass={} direction={}",
          s->GetFormID(), s->GetName(), c.school, int(s->GetCastingType()),
          int(s->GetDelivery()), c.projectile, c.form, c.direction);
    }
  spdlog::default_logger()->flush();
}
RE::TESForm* swingWeapon();
RE::SpellItem* recentSpell(RE::ActorValue);
RE::TESForm* selectedCraft() {
  auto menu = RE::UI::GetSingleton()->GetMenu<RE::CraftingMenu>();
  if (!menu) return nullptr;
  auto sub = menu->GetCraftingSubMenu();
  if (auto smith = skyrim_cast<RE::CraftingSubMenus::SmithingMenu*>(sub)) {
    if (smith->currentIndex < smith->recipes.size())
      return smith->recipes[smith->currentIndex].item;
  }
  if (auto construct =
          skyrim_cast<RE::CraftingSubMenus::ConstructibleObjectMenu*>(sub)) {
    if (construct->currentIndex < construct->recipes.size()) {
      auto recipe =
          construct->recipes[construct->currentIndex].constructibleObject;
      return recipe ? recipe->createdItem : nullptr;
    }
  }
  return nullptr;
}
bool smithingStation() {
  auto ref = RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().get();
  auto f = ref ? ref->GetBaseObject()->As<RE::TESFurniture>() : nullptr;
  if (!f) return true;
  if (!f->workBenchData.usesSkill.any(RE::ActorValue::kSmithing)) return false;
  return !(smelter && f->HasKeyword(smelter)) &&
         !(tanning && f->HasKeyword(tanning));
}
void route(RE::ActorValue value, float points, RE::TESForm* source,
           std::string_view origin, bool bash) {
  if (value == RE::ActorValue::kSmithing && !smithingStation()) {
    spdlog::debug("[ctx] smelting/tanning skipped");
    return;
  }
  auto sk = skill(value);
  if (!sk || !ready(*sk)) return;
  double raw = origin == "fixed" ? model.settings.get("debug", "fixed_xp_value")
                                 : points * sk->baseMult;
  raw *= model.settings.get("debug", "xp_mult");
  auto p = RE::PlayerCharacter::GetSingleton();
  auto left = p->GetEquippedObject(true), right = p->GetEquippedObject(false);
  auto give = [&](std::string_view id, double scale = 1.) {
    award(id, raw * scale, origin,
          origin == "fixed"
              ? model.settings.get("debug", "fixed_xp_value") * scale
              : points * scale);
  };
  if (value == RE::ActorValue::kOneHanded ||
      value == RE::ActorValue::kTwoHanded) {
    auto race = p->GetRace();
    if (race && (race == werewolf || race == vampireLord)) return;
    auto w = source ? source->As<RE::TESObjectWEAP>() : nullptr;
    if (!w) {
      auto swing = swingWeapon();
      w = swing ? swing->As<RE::TESObjectWEAP>() : nullptr;
    }
    if (!w) {
      auto a = left ? left->As<RE::TESObjectWEAP>() : nullptr,
           b = right ? right->As<RE::TESObjectWEAP>() : nullptr;
      if (a && !b)
        w = a;
      else if (b && !a)
        w = b;
    }
    if (w)
      give(weapon(w));
    else if (!left && !right)
      give("1h-unarmed");
    else {
      spdlog::warn("[ctx] unresolved weapon av={} using fallback", int(value));
      give(value == RE::ActorValue::kOneHanded ? "1h-other" : "2h-other");
    }
  } else if (value == RE::ActorValue::kBlock) {
    auto shield = left ? left->As<RE::TESObjectARMO>() : nullptr;
    bool holds = shield && shield->IsShield();
    std::string id =
        holds ? (shield->IsHeavyArmor() ? "heavy-shield" : "light-shield")
              : "parry";
    if (bash) {
      give("bash");
      if (holds) give(id, sk->bashFactor);
    } else
      give(id);
  } else if (value == RE::ActorValue::kSmithing) {
    if (!source) source = selectedCraft();
    if (!source) {
      spdlog::warn("[ctx] unresolved smithing item");
      give("other-smithing");
      return;
    }
    auto a = source->As<RE::TESObjectARMO>();
    if (a && a->IsClothing() &&
        (a->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kRing) ||
         a->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kAmulet) ||
         a->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kCirclet))) {
      give("jewelry");
      return;
    }
    std::string material = "other-smithing";
    for (auto& row : model.settings.values["materials"]["material"])
      if (has(source, keyword(row.at("keyword")))) {
        material = row.at("subskill");
        break;
      }
    give(material);
    if (auto w = source->As<RE::TESObjectWEAP>()) {
      auto sub = weapon(w);
      if (sub.starts_with("1h-"))
        give("one-handed-smithing");
      else if (sub.starts_with("2h-"))
        give("two-handed-smithing");
      else if (w->GetWeaponType() == RE::WEAPON_TYPE::kBow)
        give("bow-smithing");
      else if (w->GetWeaponType() == RE::WEAPON_TYPE::kCrossbow)
        give("crossbow-smithing");
    } else if (auto ammo = source->As<RE::TESAmmo>())
      give(ammo->IsBolt() ? "crossbow-smithing" : "bow-smithing");
    else if (a)
      give(a->IsHeavyArmor()   ? "heavy-armor-smithing"
           : a->IsLightArmor() ? "light-armor-smithing"
                               : "clothing-smithing");
  } else if (value == RE::ActorValue::kDestruction ||
             value == RE::ActorValue::kRestoration) {
    auto s = source ? source->As<RE::SpellItem>() : nullptr;
    if (source &&
        (source->As<RE::ScrollItem>() || source->As<RE::EnchantmentItem>()))
      return;
    auto hint = source ? source->As<RE::EffectSetting>() : nullptr;
    auto matches = [&](RE::SpellItem* candidate) {
      if (!candidate || classify(candidate).actorValue != value) return false;
      if (!hint) return true;
      for (auto effect : candidate->effects)
        if (effect && effect->baseEffect == hint) return true;
      return false;
    };
    if (!s) {
      auto recent = recentSpell(value);
      if (matches(recent)) s = recent;
    }
    if (!s)
      for (auto hand : {RE::MagicSystem::CastingSource::kLeftHand,
                        RE::MagicSystem::CastingSource::kRightHand,
                        RE::MagicSystem::CastingSource::kOther}) {
        auto caster = p->GetMagicCaster(hand);
        auto magic = caster ? caster->currentSpell : nullptr;
        auto spell = magic ? magic->As<RE::SpellItem>() : nullptr;
        if (matches(spell)) {
          s = spell;
          break;
        }
      }
    std::string prefix = school(value);
    if (s) {
      auto c = classify(s);
      if (c.actorValue != value) {
        spdlog::warn("[ctx] unresolved spell school mismatch");
        return;
      }
      if (!c.form.empty()) give(prefix + "/" + c.form);
      give(c.direction);
      give(prefix + "/standard");
    } else {
      spdlog::warn("[ctx] unresolved spell av={}", int(value));
      give(prefix + "/other");
      give(prefix + "/standard");
    }
  }
  synchronize();
}
}  // namespace runtime
