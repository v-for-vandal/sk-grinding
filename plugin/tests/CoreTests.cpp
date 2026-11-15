#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "Core.h"
#include "DeferredQueue.h"
#include "RuntimeMath.h"
#include "SpellRules.h"
#include "WeaponClasses.h"
using namespace sp;
void check(bool x, const char* s) {
  if (!x) throw std::runtime_error(s);
}
void near(double a, double b, double tolerance = 1e-5) {
  if (std::abs(a - b) > tolerance)
    throw std::runtime_error("expected " + std::to_string(b) + ", got " +
                             std::to_string(a));
}
int main(int argc, char** argv) try {
  // Regression for worker-thread spell XP: producers must not execute awards,
  // duplicate them, or carry queued callbacks across a cleared load boundary.
  DeferredQueue deferred;
  const auto consumer = std::this_thread::get_id();
  unsigned calls = 0;
  std::thread producer([&] {
    for (unsigned i = 0; i < 1000; ++i)
      check(deferred.post([&] {
        check(std::this_thread::get_id() == consumer,
              "main-thread consumption");
        ++calls;
      }),
            "worker event queued");
  });
  producer.join();
  check(calls == 0, "worker did not mutate progress");
  deferred.drain();
  check(calls == 1000, "all worker awards delivered exactly once");
  deferred.drain();
  check(calls == 1000, "no duplicate awards");
  deferred.post([&] { ++calls; });
  deferred.clear();
  deferred.drain();
  check(calls == 1000, "revert discards old awards");
  deferred.post([&] { deferred.post([&] { ++calls; }); });
  deferred.drain();
  check(calls == 1000, "posting during drain waits for next batch");
  deferred.drain();
  check(calls == 1001, "nested event delivered without deadlock");
  for (unsigned i = 0; i < 4096; ++i)
    check(deferred.post([] {}), "queue accepts capacity");
  check(!deferred.post([] {}), "queue overflow is reported");
  deferred.clear();
  // Shield bash previously routed to light-shield before its hit event arrived;
  // also cover the inverse order and a later ordinary block near an old bash.
  check(isBashContext(100000, 100700, 0), "bash after use");
  check(isBashContext(100700, 100000, 0), "bash before use");
  check(!isBashContext(200000, 100000, 0), "stale bash ignored");
  check(!isBashContext(100000, 99000, 100100), "closer block wins");
  check(!isBashContext(100000, 0, 0), "no hit context is not bash");
  // Missile inaccuracy must rotate the trajectory without changing speed.
  auto straight = rotateVelocity(0, 100, 0, 0, 0);
  near(straight[0], 0);
  near(straight[1], 100);
  near(straight[2], 0);
  auto offset = rotateVelocity(0, 100, 0, 0, std::acos(-1.) / 18);
  near(offset[0], 100 * std::sin(std::acos(-1.) / 18));
  near(offset[1], 100 * std::cos(std::acos(-1.) / 18));
  auto tilted = rotateVelocity(20, 80, -40, .2, -.3);
  near(tilted[0] * tilted[0] + tilted[1] * tilted[1] + tilted[2] * tilted[2],
       20 * 20 + 80 * 80 + 40 * 40);
  check(argc >= 2, "config path required");
  Model m;
  m.loadConfig(argv[1]);
  check(m.skills.size() == 6, "six skills");
  Model defaults;
  defaults.loadConfig("", true);
  check(defaults.settings.values == m.settings.values,
        "compiled defaults match repository TOML");
  if (argc == 3) {
    for (auto& sk : m.skills) m.initialize(sk, 50);
    m.initializeShared(50);
    m.pendingInit = false;
    std::ofstream(argv[2]) << m.snapshot().dump(2) << '\n';
    return 0;
  }
  auto fixture = std::filesystem::path(argv[1]).parent_path() /
                 "plugin/tests/normalization.json";
  std::ifstream ref(fixture);
  json expected;
  ref >> expected;
  for (auto& sk : m.skills) {
    near(sk.curve.between(sk.start, sk.target), expected.at(sk.av).at("N"),
         1e-6);
    for (auto& s : sk.subs)
      if (!s.fixed) near(s.k, expected.at(sk.av).at("k").at(s.group), 1e-10);
  }
  std::ifstream ini(std::filesystem::path(argv[1]) /
                    "kid/SPO_WeaponClass_KID.ini");
  std::string iniText((std::istreambuf_iterator<char>(ini)), {});
  std::regex pattern("SPO_WeapClass_[A-Za-z0-9_]+");
  std::set<std::string> names;
  for (auto it = std::sregex_iterator(iniText.begin(), iniText.end(), pattern);
       it != std::sregex_iterator(); ++it)
    names.insert(it->str());
  check(names.size() == weaponClasses.size(), "all KID keywords mapped");
  for (auto& row : weaponClasses)
    check(names.contains(std::string(row.keyword)),
          "KID keyword table mismatch");
  // WeaponClasses.h index i is the keyword with FormID 0x800 + i in the ESP.
  std::ifstream generated(std::filesystem::path(argv[1]).parent_path() /
                          "tools/esp-gen/keywords.json");
  json esp;
  generated >> esp;
  check(esp.size() == weaponClasses.size(), "esp keyword count");
  for (size_t i = 0; i < weaponClasses.size(); ++i) {
    check(esp[i].at("editorID") == std::string(weaponClasses[i].keyword),
          "WeaponClasses.h order differs from tools/esp-gen/keywords.json");
    check(std::stoul(esp[i].at("localID").get<std::string>(), nullptr, 16) ==
              0x800 + i,
          "keyword FormID differs from 0x800 + index");
  }
  {
    // A key missing in the user's SkillPractice.toml comes from the compiled
    // defaults (runtime code reads every key with .at()).
    auto dir = std::filesystem::temp_directory_path() / "sp-config-test";
    std::filesystem::remove_all(dir);
    std::filesystem::copy(argv[1], dir,
                          std::filesystem::copy_options::recursive);
    std::ifstream in(dir / "SkillPractice.toml");
    std::string text((std::istreambuf_iterator<char>(in)), {}), line, cut;
    in.close();
    std::istringstream lines(text);
    while (std::getline(lines, line))
      if (!line.starts_with("context_window_ms")) cut += line + "\n";
    std::ofstream(dir / "SkillPractice.toml") << cut;
    Model partial;
    partial.loadConfig(dir.string());
    near(partial.settings.get("xp", "context_window_ms"),
         m.settings.get("xp", "context_window_ms"));
    check(partial.settings.values["configWarnings"].dump().find(
              "xp.context_window_ms") != std::string::npos,
          "missing key warning");
    std::filesystem::remove_all(dir);
  }
  SpellShape shape;
  using D = SpellShape::Delivery;
  shape.missile = true;
  check(spellForm(shape) == "missile", "missile/lobber");
  shape.explosion = true;
  check(spellForm(shape) == "explosion", "explosive missile");
  shape.beam = true;
  check(spellForm(shape) == "beam", "beam before explosion");
  shape.delivery = D::Self;
  check(spellForm(shape) == "self", "delivery before projectile");
  shape.cloak = true;
  check(spellForm(shape) == "cloak", "cloak before self");
  shape.summon = true;
  check(spellForm(shape).empty(), "summon excluded");
  shape = {};
  shape.delivery = D::Location;
  check(spellForm(shape) == "rune", "target location heuristic");
  shape.concentration = true;
  check(spellForm(shape) == "wall", "concentration wall");
  shape = {};
  shape.flame = true;
  check(spellForm(shape) == "flame", "cone/flamethrower");
  shape = {};
  check(spellForm(shape).empty(), "unknown projectile");
  Curve c;
  LevelXP l;
  l.add(.001, c, 500);
  near(l.value(c), 1);
  l.add(c.between(1, 75.4), c, 500);
  near(l.value(c), 75.4);
  LevelXP floor;
  floor.set(20.5, c, 500);
  l.subtract(1000, c, floor);
  near(l.value(c), 20.5);
  l.add(1e30, c, 500);
  near(l.value(c), 500);
  near(l.progress, 0);
  for (auto& sk : m.skills) {
    double n = sk.curve.between(sk.start, sk.target);
    near(sk.curve.levelFromXP(sk.start, n), sk.target);
    near(sk.curve.levelFromXP(75.3, -sk.curve.between(15.7, 75.3)), 15.7);
    double path = 0;
    for (auto& s : sk.subs)
      if (std::find(sk.path.begin(), sk.path.end(), s.id) != sk.path.end())
        path += s.worth(sk.pathLevel);
    near(path, n, 1e-6);
    m.initialize(sk, 50);
    near(sk.computed(0, 300), 50);
    for (auto& s : sk.subs)
      near(s.current.value(s.curve), (s.fixed && s.k == 0) ? 0 : 70);
  }
  for (double u : {0., 20., 30., 70., 90.})
    for (double dt : {.1, 10., 80.}) {
      double numeric = 0;
      int steps = 100000;
      for (int i = 0; i < steps; ++i)
        numeric += ramp(u + (i + .5) * dt / steps, 24, 72) * dt / steps;
      near(rampIntegral(u, dt, 24, 72), numeric, 1e-6);
    }
  // The zone and decay checks below are about the own floor: no cross-links here (they
  // are covered by the cross-link tests further down).
  for (auto sk : m.all())
    for (auto& x : sk->subs) x.links.clear();
  auto* s = m.find("1h-sword");
  s->curve = {1, 10, 1};
  s->current.set(5, s->curve, 500);
  s->floor.set(10, s->curve, 500);
  s->potential.set(20, s->curve, 500);
  s->peak = s->potential;
  double raw = s->curve.between(5, 10) / 3 + s->curve.between(10, 20) / 1.5 +
               s->curve.between(20, 22);
  m.award(s->id, raw);
  near(s->current.value(s->curve), 22);
  near(s->potential.value(s->curve), 22);
  check(s->floor.value(s->curve) <= 22, "floor<=current");
  double permanent = s->floor.value(s->curve);
  s->current.set(1, s->curve, 500);
  m.award(s->id, 1);
  check(s->floor.value(s->curve) >= permanent,
        "debug low current never erases permanent floor");
  s->current.set(22, s->curve, 500);
  auto saved = m.save();
  saved["subs"]["removed"] = {{"test", 42}};
  saved["mains"]["removed"] = {{"test", 7}};
  Model loaded;
  loaded.loadConfig(argv[1]);
  loaded.restore(saved);
  check(loaded.save() == saved, "save roundtrip/unknown IDs");
  LevelXP below;
  below.set(3, c, 500);
  below.subtract(1, c, floor);
  near(below.value(c), 3);
  auto removed = loaded;
  auto& list = removed.skills[0].subs;
  auto removedID = list[0].id;
  list.erase(list.begin());
  auto temporary = removed.save();
  Model returned;
  returned.loadConfig(argv[1]);
  returned.restore(temporary);
  check(returned.save()["subs"][removedID] == saved["subs"][removedID],
        "removed subskill survives save and re-add");
  auto added = saved;
  added["subs"].erase("1h-mace");
  for (auto& [key, v] : added["mains"].items())
    if (v.contains("initial")) v["initial"].erase("1h-mace");
  Model fresh;
  fresh.loadConfig(argv[1]);
  fresh.restore(added);
  near(fresh.find("1h-mace")->current.value(fresh.find("1h-mace")->curve), 70);
  check(fresh.save()["subs"]["1h-sword"] == saved["subs"]["1h-sword"],
        "new subskill preserves existing progress");
  m.settings.values["debug"]["no_grace"] = true;
  m.settings.values["debug"]["time_scale"] = 1000;
  m.tick(24);
  near(s->current.value(s->curve), s->floor.value(s->curve));
  check(m.skills[0].rust >= 0, "rust nonnegative");
  Model dynamics;
  dynamics.loadConfig(argv[1]);
  for (auto& sk : dynamics.skills) dynamics.initialize(sk, 15);
  sp::Skill* oh = nullptr;
  auto sword = dynamics.find("1h-sword", &oh);
  sword->current.set(75, sword->curve, 500);
  sword->peak = sword->potential = sword->current;
  double beforeF = oh->computed(0, 300);
  dynamics.settings.values["debug"]["no_grace"] = true;
  dynamics.tick(24);
  near(sword->current.value(sword->curve), 71);
  near(oh->rust, 2);
  check(oh->computed(0, 300) < beforeF, "neglected main rusts");
  near(dynamics.activeHours, 24);
  double rusty = oh->rust;
  dynamics.award("1h-mace", 1000);
  check(oh->rust < rusty, "mace practice reduces OneHanded rust");
  {
    // One vanilla award feeding three sub-skills lowers rust once, by its raw.
    auto once = dynamics, single = dynamics;
    sp::Skill *a = nullptr, *b = nullptr;
    once.find("1h-sword", &a);
    single.find("1h-sword", &b);
    double before = a->rust;
    for (auto id : {"1h-sword", "1h-mace", "1h-dagger"})
      once.award(id, 300, false);
    near(a->rust, before);
    once.recover(*a, 300);
    single.award("1h-sword", 300);
    near(a->rust, b->rust, 1e-12);
    near(a->idle, b->idle, 1e-12);
    check(a->rust < before, "recover lowers rust");
  }
  auto split = dynamics, whole = dynamics;
  split.tick(10);
  split.tick(20);
  whole.tick(30);
  near(split.find("1h-sword")->current.value(sword->curve),
       whole.find("1h-sword")->current.value(sword->curve), 1e-5);
  near(split.skills[2].rust, whole.skills[2].rust, 1e-5);
  {
    // Shared magic sub-skills: pseudo-skill "magic", forms start from the best school,
    // archetypes at 0, no rust, old per-school records are dropped at restore.
    Model magic;
    magic.loadConfig(argv[1]);
    check(magic.shared.pseudo && magic.shared.subs.size() == 11, "shared sub-skills");
    check(!magic.find("destruction/missile"), "no per-school forms");
    sp::Skill* parent = nullptr;
    auto missile = magic.find("form/missile", &parent);
    check(missile && parent == &magic.shared && missile->k == 0, "form/missile is shared");
    for (auto& sk : magic.skills) magic.initialize(sk, 40);
    magic.initializeShared(50);
    near(missile->current.value(missile->curve), 70);
    near(magic.find("archetype/standard")->current.value(missile->curve), 0);
    magic.award("form/missile", 5000);
    check(missile->current.value(missile->curve) > 70, "shared sub-skill gains XP");
    near(magic.shared.rust, 0);
    magic.settings.values["debug"]["no_grace"] = true;
    magic.tick(48);
    near(magic.shared.rust, 0);
    auto snap = magic.snapshot();
    check(snap["skills"].back()["av"] == "magic" && snap["skills"].back()["pseudo"] == true &&
              !snap["skills"].back().contains("F"),
          "pseudo-skill in the snapshot");
    auto old = magic.save();
    check(old["mains"].contains("magic"), "shared state saved");
    old["subs"]["destruction/missile"] = old["subs"]["form/missile"];
    old["mains"]["Destruction"]["initial"]["destruction/missile"] = 70;
    Model reloaded;
    reloaded.loadConfig(argv[1]);
    reloaded.restore(old);
    check(reloaded.dropped == std::vector<std::string>{"destruction/missile"},
          "old per-school form dropped");
    check(!reloaded.save()["subs"].contains("destruction/missile"), "dropped from save");
    near(reloaded.find("form/missile")->current.value(missile->curve),
         missile->current.value(missile->curve));
  }
  {
    // Cross-links (design-v2 §6, impl/cross-links.md).
    Model x;
    x.loadConfig(argv[1]);
    for (auto& sk : x.skills) x.initialize(sk, 15);  // every floor 0
    x.initializeShared(15);
    x.pendingInit = false;
    auto lvl = [&](const char* id, double current, double floor) {
      auto s = x.find(id);
      s->current.set(current, s->curve, 500);
      s->floor.set(floor, s->curve, 500);
      s->potential = s->peak = s->current;
    };
    lvl("1h-sword", 80, 60);
    std::string from;
    near(x.effectiveFloor(*x.find("1h-mace"), &from), 30);
    check(from == "1h-sword", "1h link source");
    near(x.effectiveFloor(*x.find("2h-sword"), &from), 15);
    near(x.effectiveFloor(*x.find("1h-unarmed")), 30);
    near(x.effectiveFloor(*x.find("dual-wield"), &from), 0);
    check(from == "own" && x.find("dual-wield")->links.empty(), "concept sub-skills unlinked");
    check(x.find("heavy-shield")->links.empty(), "Block has no links");
    lvl("1h-dagger", 50, 40);  // non-cumulative: 0.5 × 60 and 0.5 × 40 do not add up
    near(x.effectiveFloor(*x.find("1h-mace")), 30);
    lvl("1h-mace", 0, 50);  // own floor above the linked ones wins
    near(x.effectiveFloor(*x.find("1h-mace"), &from), 50);
    check(from == "own", "own floor source");
    lvl("1h-mace", 0, 0);
    {
      // Non-transitive: sword → mace → dagger gives the dagger nothing from the sword.
      Model chain = x;
      for (auto sk : chain.all())
        for (auto& s : sk->subs) s.links.clear();
      auto ref = [&](const char* id) {
        for (int k = 0; k < int(chain.skills.size()); ++k)
          for (int i = 0; i < int(chain.skills[k].subs.size()); ++i)
            if (chain.skills[k].subs[i].id == id) return LinkIn{k, i, .5};
        throw std::runtime_error("no such sub-skill");
      };
      chain.find("1h-mace")->links = {ref("1h-sword")};
      chain.find("1h-dagger")->links = {ref("1h-mace")};
      chain.find("1h-dagger")->floor.set(0, chain.find("1h-dagger")->curve, 500);
      near(chain.effectiveFloor(*chain.find("1h-mace")), 30);
      near(chain.effectiveFloor(*chain.find("1h-dagger")), 0);
    }
    // Catch-up zone up to the effective floor, then the learning zone; the loop ends.
    auto mace = x.find("1h-mace");
    auto snapSub = [&](const char* id) {
      json snap = x.snapshot();
      for (auto& sk : snap["skills"])
        for (auto& s : sk["subs"])
          if (s["id"] == id) return s;
      throw std::runtime_error("not in snapshot");
    };
    check(snapSub("1h-mace")["zone"] == 3.0, "catch-up below eff. floor");
    x.award("1h-mace", mace->curve.between(0, 30) / 3 + mace->curve.between(30, 31));
    near(mace->current.value(mace->curve), 31, 1e-4);
    auto sub = snapSub("1h-mace");
    check(sub["floorFrom"] == "1h-sword", "snapshot floorFrom");
    near(sub["effFloor"].get<double>(), 30);
    check(sub["ownFloor"].get<double>() < 30, "own floor below the link");
    // Decay stops at the effective floor, not at the own floor.
    lvl("1h-mace", 50, 0);
    x.settings.values["debug"]["no_grace"] = true;
    x.settings.values["debug"]["time_scale"] = 1000;
    x.tick(24);
    near(mace->current.value(mace->curve), 30, 1e-4);
    // Alteration: 0.2 × 2 × level on every magic sub-skill (schools and shared), not on 1h.
    x.externalLevels["Alteration"] = 50;
    near(x.effectiveFloor(*x.find("destruction/fire"), &from), 20);
    check(from == "Alteration", "Alteration source");
    near(x.effectiveFloor(*x.find("form/missile")), 20);
    near(x.effectiveFloor(*x.find("archetype/standard")), 20);
    check(x.find("restoration/help")->magic, "Restoration sub-skills are magic");
    check(!x.find("1h-sword")->magic, "weapons are not magic");
    sp::Skill* oneHanded = nullptr;
    x.find("1h-sword", &oneHanded);
    near(x.mainLevel("OneHanded"), oneHanded->written);
    near(x.mainLevel("Speech"), 0);
  }
  {
    // Requirement formulas (impl/perk-requirements.md).
    NameResolver names;
    names.ids = {"a-1", "a-2", "a-3", "b"};
    names.aliases["line"] = {"a-*"};
    names.aliases["pair"] = {"a-1", "b"};
    std::map<std::string, double> v = {{"a-1", 30}, {"a-2", 20}, {"a-3", 10}, {"b", 0}};
    double main = 40;
    Values vals{[&](const std::string& id) { return v.at(id); },
                [&](const std::string&) { return main; }};
    std::vector<std::string> notes;
    auto ok = [&](const char* f, double slack = 0) {
      return evaluate(parseRequirement(f, names, notes), vals, slack);
    };
    check(ok("oneOf(line) >= 30") && !ok("oneOf(line) >= 31"), "oneOf");
    check(ok("twoOf(line) >= 20") && !ok("twoOf(line) >= 21"), "twoOf");
    check(ok("threeOf(line) >= 10") && !ok("threeOf(line) >= 11"), "threeOf");
    check(ok("twoOf(a-*) >= 20") && ok("oneOf(pair) >= 30") && ok("line >= 30"), "globs, aliases");
    check(ok("a-1 >= 30") && !ok("b >= 1"), "single sub-skill");
    check(ok("OneHanded >= 40") && !ok("HeavyArmor >= 41"), "main-skill operand");
    check(ok("b >= 5 OR a-1 >= 5 AND a-2 >= 5"), "AND binds tighter than OR");
    check(!ok("(b >= 5 OR a-1 >= 5) AND a-3 >= 15"), "parentheses");
    check(ok("b >= 5 or a-1 >= 5 and a-2 >= 5"), "keywords in any case");
    check(ok("oneOf(line) >= 31", 2) && !ok("oneOf(line) >= 33", 2), "slack lowers thresholds");
    check(notes.empty(), "no notes yet");
    ok("twoOf(b) >= 1");
    check(notes.size() == 1, "k > n is a warning");
    for (auto bad : {"b >", "oneOf(b >= 1", "b >= 1 junk", "b = 1", "(b >= 1", "c >= 1",
                     "zz-* >= 1", "b >= x"}) {
      bool threw = false;
      try {
        parseRequirement(bad, names, notes);
      } catch (const std::runtime_error&) {
        threw = true;
      }
      check(threw, bad);
    }
    auto tree = explain(parseRequirement("b >= 5 OR twoOf(line) >= 20", names, notes), vals);
    check(tree["op"] == "or" && tree["met"] == true && tree["terms"][1]["values"].size() == 3,
          "explain tree");
  }
  {
    // config/perks/*.toml: every row parses; hysteresis; state survives save/restore.
    Model r;
    r.loadConfig(argv[1]);
    for (auto& sk : r.skills) r.initialize(sk, 15);
    r.initializeShared(15);
    check(r.requirements.size() == 26 + 16, "all perk rows loaded");
    for (auto& w : r.settings.values["configWarnings"])
      check(w.get<std::string>().find("perks/") == std::string::npos, "no perk row skipped");
    auto idx = [&](const char* h) {
      for (size_t i = 0; i < r.requirements.size(); ++i)
        if (r.requirements[i].handle == h) return i;
      throw std::runtime_error(std::string("no requirement ") + h);
    };
    check(r.requirements[idx("1H-05")].global == "SPO_Req_1H05" &&
              r.requirements[idx("ORD-LA-05")].global == "SPO_Req_ORDLA05",
          "global names");
    auto set = [&](const char* id, double l) {
      auto s = r.find(id);
      s->current.set(l, s->curve, 500);
    };
    auto changed = [&](const char* h) {
      auto c = r.updateRequirements();
      return std::find(c.begin(), c.end(), idx(h)) != c.end();
    };
    check(r.updateRequirements().empty(), "nothing met at level 0");
    set("1h-mace", 45);
    check(changed("1H-05") && r.requirements[idx("1H-05")].met, "1H-05 met at 45");
    set("1h-mace", 43.5);
    check(!changed("1H-05") && r.requirements[idx("1H-05")].met, "kept within hysteresis");
    auto saved = r.save();
    set("1h-mace", 42.9);
    check(changed("1H-05") && !r.requirements[idx("1H-05")].met, "lost below 45 - 2");
    Model back;
    back.loadConfig(argv[1]);
    back.restore(saved);
    check(back.requirements[idx("1H-05")].met, "met flag restored");
    // Sword line: two of sword / short sword / spear at 35 (1H-07: 45 / 35 / 25).
    set("1h-sword", 35);
    r.updateRequirements();
    check(!r.requirements[idx("1H-07")].met, "one at 35 is not enough");
    set("1h-spear", 35);
    r.updateRequirements();
    check(r.requirements[idx("1H-07")].met, "two at 35");
    // BLK-10: shield/parry 10 AND 1h 10/20/30.
    set("parry", 10);
    r.updateRequirements();
    check(r.requirements[idx("BLK-10")].met, "BLK-10 met");
    for (auto id : {"1h-sword", "1h-spear", "1h-mace"}) set(id, 0);
    set("1h-dagger", 29);  // oneOf 30 − hysteresis 2
    r.updateRequirements();
    check(r.requirements[idx("BLK-10")].met, "BLK-10 kept at 29");
    set("1h-dagger", 27);
    r.updateRequirements();
    check(!r.requirements[idx("BLK-10")].met, "BLK-10 lost at 27");
  }
  {
    // A row naming an unmanaged sub-skill is skipped with a warning; a syntax error is fatal.
    auto dir = std::filesystem::temp_directory_path() / "sp-core-tests-perks";
    std::filesystem::remove_all(dir);
    std::filesystem::copy(argv[1], dir, std::filesystem::copy_options::recursive);
    std::ofstream(dir / "perks" / "zz.toml")
        << "[[perk]]\nhandle = \"LA-10\"\nperk = \"000001:Test.esp\"\n"
           "requires = \"clothing >= 30\"\n";
    Model u;
    u.loadConfig(dir.string());
    check(u.settings.values["configWarnings"].dump().find("LA-10 skipped") != std::string::npos,
          "unknown sub-skill row skipped");
    std::ofstream(dir / "perks" / "zz.toml")
        << "[[perk]]\nhandle = \"LA-10\"\nperk = \"000001:Test.esp\"\n"
           "requires = \"1h-sword >=\"\n";
    bool threw = false;
    try {
      Model bad;
      bad.loadConfig(dir.string());
    } catch (const std::runtime_error&) {
      threw = true;
    }
    check(threw, "syntax error in a perk row is fatal");
    std::filesystem::remove_all(dir);
  }
  std::cout
      << "Core tests passed: six normalization paths, fractional curves, cap, "
         "zones, floors, decay integral, initialization, persistence, shared magic "
         "sub-skills, cross-links, Alteration floor, requirement formulas.\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
