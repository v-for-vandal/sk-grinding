#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "Requirements.h"
namespace sp {
using json = nlohmann::json;
struct Curve {
  double mult = 2, offset = 0, exponent = 1.95;
  double cost(int level) const;
  double between(double from, double to) const;
  double levelFromXP(double base, double xp, int cap = 500) const;
};
struct LevelXP {
  std::uint16_t level = 0;
  float progress = 0;
  double value(const Curve&) const;
  void set(double value, const Curve&, int cap);
  void add(double xp, const Curve&, int cap);
  void subtract(double levels, const Curve&, const LevelXP& floor);
};
// Cross-link into a sub-skill (design-v2 §6, impl/cross-links.md): the own floor of the
// sub-skill at (skill, sub), an index into Model::all(), times c.
struct LinkIn {
  int skill = 0, sub = 0;
  double c = 0;
};
struct Sub {
  std::string id, group;
  Curve curve;
  double T = 90, w = 15, T1 = 25, k = 0, decay = 0, initial = 0;
  bool fixed = false;
  std::vector<double> integral;
  LevelXP current, floor, potential, peak;
  double idle = 0, lastXP = 0;
  // Shown in the panel only (impl/status-ui.md *Effects and automatic perks*):
  // continuous effects (from [[effect]] and [aim]) and automatic perks.
  json effects = json::array(), perks = json::array();
  std::vector<LinkIn> links;  // non-transitive: only own floors are read
  bool magic = false;         // its effective floor follows the main Alteration level
  double shape(double level) const;
  void prepare(int cap);
  double worth(double level) const;
};
struct Skill {
  std::string av;
  Curve curve;
  double start = 15, target = 100, pathLevel = 75, baseMult = 1, decay = 1,
         rustRate = .5, paceSub = 2000, paceSkill = 1000, bashFactor = .5;
  std::vector<std::string> path;
  std::map<std::string, double> budgets;
  std::vector<Sub> subs;
  bool initialized = false;
  // The shared magic sub-skills (config/skills/shared.toml): no main skill, k = 0, no rust;
  // shown as the pseudo-skill "magic" (impl/status-ui.md).
  bool pseudo = false;
  std::string name;
  double base = 15, rust = 0, idle = 0, written = 15;
  unsigned external = 0;
  void solve(int cap);
  double worthDelta() const;
  double computed(double minimum, double maximum) const;
};
struct Settings {
  json values;
  double get(const char* section, const char* key) const;
  bool flag(const char* section, const char* key) const;
};
// One perk requirement of config/perks/*.toml (impl/perk-requirements.md): one formula, one
// GlobalVariable of our ESL that the DLL sets to 1 / 0.
struct Requirement {
  std::string handle, global, formula, file;
  json perk;  // the whole config row (FormKey, EditorID, main_level, …) for the runtime
  Expr expr;
  bool met = false;
};
struct AlterationFloor {
  bool enabled = false;
  double c = .2, scale = 2;
};
struct Model {
  Settings settings;
  std::vector<Skill> skills;
  Skill shared;  // pseudo-skill "magic"; empty subs when shared.toml is absent
  std::vector<std::string> dropped;  // old per-school form records dropped at the last restore
  double activeHours = 0, lastHours = 0;
  bool pendingInit = true, raceClosed = false;
  json unknownSubs = json::object(), unknownMains = json::object();
  // Levels of main skills the plugin does not manage (Alteration, HeavyArmor, Speech, …),
  // set by the runtime from the player's base actor values; 0 when absent.
  std::map<std::string, double> externalLevels;
  AlterationFloor alteration;
  std::vector<Requirement> requirements;
  void loadConfig(const std::string& root, bool builtIn = false);
  void initialize(Skill&, double base);
  // Shared sub-skills: forms start like a school's sub-skills at the best school level,
  // archetypes at 0 (impl/main-skill-level.md).
  void initializeShared(double bestSchool);
  // Start level of a sub-skill (and of a form, from the best school): the init mapping.
  double startLevel(double base) const;
  std::vector<Skill*> all();  // skills, then shared when it has sub-skills
  std::vector<const Skill*> all() const;
  Sub* find(const std::string& id, Skill** parent = nullptr);
  // recover = false: the caller lowers rust once per vanilla event (recover()).
  void award(const std::string& id, double raw, bool recover = true);
  void recover(Skill&, double raw);
  void tick(double hours);
  json save() const;
  void restore(const json&);
  json snapshot() const;
  json subSnapshot(const Skill&, const Sub&, double grace, double rampEnd) const;
  json effects(const Sub&) const;
  const Sub& at(const LinkIn&) const;
  // Written level of a managed main skill, else externalLevels (0 when unknown).
  double mainLevel(const std::string& av) const;
  // max(own floor, c × own floor of each linked sub-skill, Alteration term); `source` gets
  // "own", the linked sub-skill id or "Alteration".
  double effectiveFloor(const Sub&, std::string* source = nullptr) const;
  Values values() const;
  // Re-evaluates every requirement with hysteresis; returns the indices whose state changed.
  std::vector<size_t> updateRequirements();
  json requirementSnapshot() const;
  json perks(const Sub&, double current) const;
};
double ramp(double idle, double grace, double end);
// Written integer level n after hysteresis (impl/main-skill-level.md).
double hysteresis(double F, double n, double up, double down);
double rampIntegral(double idle, double dt, double grace, double end);
}  // namespace sp
