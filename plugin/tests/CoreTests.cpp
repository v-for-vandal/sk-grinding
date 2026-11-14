#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

#include "Core.h"
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
  std::cout
      << "Core tests passed: six normalization paths, fractional curves, cap, "
         "zones, floors, decay integral, initialization, persistence.\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << e.what() << '\n';
  return 1;
}
