#include <filesystem>
#include <set>
#include <toml++/toml.hpp>

#include "Core.h"
#include "Defaults.h"
namespace sp {
namespace {
json convert(const toml::node& n) {
  if (auto t = n.as_table()) {
    json j = json::object();
    for (auto& [k, v] : *t) j[std::string(k.str())] = convert(v);
    return j;
  }
  if (auto a = n.as_array()) {
    json j = json::array();
    for (auto& v : *a) j.push_back(convert(v));
    return j;
  }
  if (n.is_boolean()) return *n.value<bool>();
  if (n.is_integer()) return *n.value<std::int64_t>();
  if (n.is_floating_point()) {
    double v = *n.value<double>();
    if (!std::isfinite(v)) throw std::runtime_error("non-finite TOML value");
    return v;
  }
  if (n.is_string()) return *n.value<std::string>();
  throw std::runtime_error("unsupported TOML value");
}
Curve curve(const json& j, Curve c = {}) {
  c.mult = j.value("improve_mult", c.mult);
  c.offset = j.value("improve_offset", c.offset);
  c.exponent = j.value("exponent", c.exponent);
  if (c.mult < 0 || c.offset < 0 || c.exponent <= 0)
    throw std::runtime_error("invalid curve");
  return c;
}
// Keys of `defaults` missing in `user`, as dotted paths (only tables recurse).
void missing(const json& defaults, const json& user, const std::string& prefix,
             std::vector<std::string>& out) {
  for (auto& [key, value] : defaults.items()) {
    auto path = prefix.empty() ? key : prefix + "." + key;
    if (!user.is_object() || !user.contains(key))
      out.push_back(path);
    else if (value.is_object())
      missing(value, user.at(key), path, out);
  }
}
void requireRows(const json& file, const char* table, const char* list,
                 const char* name) {
  if (!file.contains(table)) return;
  if (!file.at(table).is_array())
    throw std::runtime_error(std::string(name) + ": [[" + table +
                             "]] is not an array of tables");
  for (auto& row : file.at(table)) {
    if (!row.contains("subskill") || !row.at("subskill").is_string() ||
        !row.contains(list))
      throw std::runtime_error(std::string(name) + ": row without subskill/" +
                               list);
    if (std::string_view(list) == "keyword" ? !row.at(list).is_string()
                                            : !row.at(list).is_array())
      throw std::runtime_error(std::string(name) + ": bad " + list);
  }
}
// [[effect]] and [[auto_perk]] of a skill file: shown in the panel only
// (impl/status-ui.md *Effects and automatic perks*); the DLL applies none of them yet.
void readDisplay(const json& file, Skill& sk, const std::string& name) {
  auto bad = [&](const std::string& what) {
    throw std::runtime_error(name + ": " + what);
  };
  auto find = [&](const std::string& id) -> Sub& {
    for (auto& s : sk.subs)
      if (s.id == id) return s;
    bad("unknown subskill " + id);
    throw std::logic_error("unreachable");
  };
  for (auto& e : file.value("effect", json::array())) {
    if (!e.contains("name") || !e.at("name").is_string()) bad("effect without name");
    json out = {{"name", e.at("name")},
                {"unit", e.value("unit", "%")},
                {"invert", e.value("invert", false)},
                {"applied", false}};
    if (e.contains("curve")) {
      auto& c = e.at("curve");
      out["kind"] = "curve";
      for (auto key : {"pen", "P", "M", "h"}) out[key] = c.at(key).get<double>();
      out["p"] = c.value("p", 1.);
      if (out["P"] <= 0 || out["h"] <= 0 || out["p"] <= 0) bad("bad effect curve");
    } else if (e.contains("linear")) {
      auto& c = e.at("linear");
      out["kind"] = "linear";
      out["at0"] = c.at("at0").get<double>();
      out["zeroAt"] = c.at("zero_at").get<double>();
      if (out["zeroAt"] <= 0) bad("bad effect line");
    } else
      bad("effect without curve or linear");
    std::vector<std::string> targets;
    if (e.contains("subskills"))
      targets = e.at("subskills").get<std::vector<std::string>>();
    else if (e.contains("group")) {
      for (auto& s : sk.subs)
        if (s.group == e.at("group").get<std::string>()) targets.push_back(s.id);
    } else
      bad("effect without group or subskills");
    if (targets.empty()) bad("effect without targets");
    for (auto& id : targets) find(id).effects.push_back(out);
  }
  for (auto& p : file.value("auto_perk", json::array())) {
    for (auto key : {"id", "name", "subskill", "description"})
      if (!p.contains(key) || !p.at(key).is_string())
        bad(std::string("auto_perk without ") + key);
    double level = p.at("level");
    if (level < 0) bad("auto_perk level");
    find(p.at("subskill")).perks.push_back(
        {{"id", p.at("id")},
         {"name", p.at("name")},
         {"level", level},
         {"description", p.at("description")}});
  }
}
}  // namespace
void Model::loadConfig(const std::string& root, bool builtIn) {
  Model next;
  auto parse = [&](const std::string& name) {
    return convert(builtIn ? toml::parse(defaults.at(name), name)
                           : toml::parse_file(root + "/" + name));
  };
  // Runtime code reads settings with .at(): a key missing in the user's file
  // must not throw later inside a game hook. Fill it from the compiled copy.
  auto settings = parse("SkillPractice.toml");
  std::vector<std::string> absent;
  if (!builtIn) {
    auto compiled =
        convert(toml::parse(std::string_view(defaults.at("SkillPractice.toml")),
                              std::string_view("defaults")));
    missing(compiled, settings, "", absent);
    compiled.merge_patch(settings);
    settings = std::move(compiled);
  }
  next.settings.values = std::move(settings);
  next.settings.values["configWarnings"] = json::array();
  for (auto& key : absent)
    next.settings.values["configWarnings"].push_back(
        "SkillPractice.toml: missing " + key + ", using built-in default");
  int cap = int(next.settings.get("xp", "hard_cap"));
  if (next.settings.get("aim", "missile_L0") <= 0 ||
      next.settings.get("aim", "beam_L0") <= 0 ||
      next.settings.get("aim", "beam_t_full_sec") <= 0 ||
      next.settings.get("aim", "beam_secondary_weight") < 0)
    throw std::runtime_error("invalid aim settings");
  if (cap < 1 || cap > 65534 || next.settings.get("decay", "q") <= 0 ||
      next.settings.get("xp", "zone_new") <= 0 ||
      next.settings.get("xp", "zone_recall") <= 0 ||
      next.settings.get("xp", "zone_catch_up") <= 0 ||
      next.settings.get("main", "hysteresis_up") <= 0 ||
      next.settings.get("main", "hysteresis_down") < 0 ||
      next.settings.get("decay", "tick_seconds") <= 0 ||
      next.settings.get("decay", "idle_max_hours") <
          next.settings.get("decay", "ramp_end_hours"))
    throw std::runtime_error("invalid XP/decay configuration");
  std::set<std::string> ids, avs;
  std::vector<std::filesystem::path> files;
  if (builtIn) {
    for (auto& [name, content] : defaults)
      if (name.starts_with("skills/")) files.emplace_back(name);
  } else
    for (auto& f : std::filesystem::directory_iterator(root + "/skills"))
      if (f.path().extension() == ".toml")
        files.push_back(std::filesystem::path("skills") / f.path().filename());
  std::sort(files.begin(), files.end());
  for (auto& path : files) {
    auto j = parse(path.generic_string());
    if (!j.contains("main")) {
      // shared.toml: the shared magic sub-skills, pseudo-skill "magic" (k = 0, no rust).
      if (!j.contains("shared") || !next.shared.subs.empty())
        throw std::runtime_error(path.generic_string() +
                                 ": no [main] and not the one shared file");
      auto& sh = next.shared;
      sh.pseudo = true;
      sh.av = "magic";
      sh.name = j.at("shared").value("name", "Magic");
      auto d = j.value("dynamics", json::object());
      sh.decay = d.value("decay_rate", 2.);
      sh.paceSub = d.value("pace_sub", 1500.);
      sh.rustRate = 0;
      if (sh.decay < 0 || sh.paceSub <= 0)
        throw std::runtime_error("invalid shared dynamics");
      for (auto& v : j.at("subskill")) {
        Sub s;
        s.id = v.at("name");
        if (!ids.insert(s.id).second)
          throw std::runtime_error("duplicate subskill " + s.id);
        s.group = v.at("group");
        s.T = v.value("T", 90.);
        s.w = v.value("w", 15.);
        s.fixed = true;
        s.k = 0;
        s.decay = v.value("decay_rate", sh.decay);
        s.curve = curve(v.value("curve", json::object()),
                        curve(j.value("sub_curve", json::object())));
        if (s.w <= 0 || s.decay < 0) throw std::runtime_error("invalid subskill");
        s.prepare(cap);
        sh.subs.push_back(s);
      }
      readDisplay(j, sh, path.generic_string());
      continue;
    }
    Skill sk;
    auto m = j.at("main"), d = j.at("dynamics");
    sk.av = m.at("actor_value");
    if (!avs.insert(sk.av).second)
      throw std::runtime_error("duplicate skill " + sk.av);
    if (sk.av != "OneHanded" && sk.av != "TwoHanded" && sk.av != "Block" &&
        sk.av != "Smithing" && sk.av != "Destruction" && sk.av != "Restoration")
      throw std::runtime_error("unsupported prototype skill " + sk.av);
    sk.curve = curve(m);
    sk.start = m.at("start_level");
    sk.target = m.at("target_level");
    sk.baseMult = d.at("base_mult");
    sk.decay = d.at("decay_rate");
    sk.rustRate = d.at("rust_rate");
    sk.paceSub = d.at("pace_sub");
    sk.paceSkill = d.at("pace_skill");
    if (sk.paceSub <= 0 || sk.paceSkill <= 0 || sk.decay < 0 || sk.rustRate < 0)
      throw std::runtime_error("invalid dynamics");
    sk.bashFactor =
        j.value("routing", json::object()).value("bash_shield_factor", .5);
    sk.pathLevel = j.at("reference_path").at("level");
    sk.path =
        j.at("reference_path").at("members").get<std::vector<std::string>>();
    double remain = sk.curve.between(sk.start, sk.target), shares = 0;
    for (auto& [g, b] : j.at("budget").items())
      if (b.contains("main_from")) {
        double n = sk.curve.between(b.at("main_from"), b.at("main_to"));
        sk.budgets[g] = n;
        remain -= n;
      } else
        shares += b.at("share").get<double>();
    if (remain < 0) throw std::runtime_error("absolute budgets exceed target");
    for (auto& [g, b] : j.at("budget").items())
      if (b.contains("share")) {
        if (shares <= 0) throw std::runtime_error("invalid shares");
        sk.budgets[g] = remain * b.at("share").get<double>() / shares;
      }
    for (auto& v : j.at("subskill")) {
      Sub s;
      s.id = v.at("name");
      if (!ids.insert(s.id).second) {
        next.settings.values["configWarnings"].push_back(
            "duplicate subskill ignored: " + s.id);
        continue;
      }
      s.group = v.at("group");
      s.T = v.at("T");
      s.w = v.at("w");
      s.T1 = v.value("T1", j.at("conversion").value("rise_T1", 25.));
      s.fixed = v.contains("k");
      s.k = v.value("k", 0.);
      s.decay = v.value("decay_rate", sk.decay);
      s.curve =
          curve(v.value("curve", json::object()), curve(j.at("sub_curve")));
      if (s.w <= 0 || s.decay < 0) throw std::runtime_error("invalid subskill");
      if (!s.fixed && !sk.budgets.contains(s.group))
        throw std::runtime_error("missing group budget");
      sk.subs.push_back(s);
    }
    for (auto& id : sk.path)
      if (std::none_of(sk.subs.begin(), sk.subs.end(),
                       [&](auto& s) { return s.id == id; }))
        throw std::runtime_error("unknown path member " + id);
    readDisplay(j, sk, path.generic_string());
    sk.solve(cap);
    next.skills.push_back(sk);
  }
  if (next.skills.empty()) throw std::runtime_error("no skill configurations");
  next.settings.values["materials"] = parse("smithing-materials.toml");
  next.settings.values["directions"] = parse("magic-directions.toml");
  requireRows(next.settings.values["materials"], "material", "keyword",
              "smithing-materials.toml");
  for (auto school : {"destruction", "restoration"})  // read at runtime
    requireRows(next.settings.values["directions"], school, "keywords",
                "magic-directions.toml");
  if (!skills.empty()) next.restore(save());
  *this = std::move(next);
}
}  // namespace sp
