#include "Core.h"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
namespace sp {
double Curve::cost(int l) const {
  return std::max(.001, mult * std::pow(std::max(0, l), exponent) + offset);
}
double Curve::between(double a, double b) const {
  if (b < a) return -between(b, a);
  double out = 0;
  while (a < b) {
    int n = static_cast<int>(std::floor(a));
    double step = std::min(b - a, n + 1 - a);
    out += step * cost(n);
    a += step;
  }
  return out;
}
double Curve::levelFromXP(double base, double xp, int cap) const {
  double l = std::clamp(base, 0., double(cap));
  while (xp > 0 && l < cap) {
    int n = int(std::floor(l));
    double need = (n + 1 - l) * cost(n);
    if (xp < need) return l + xp / cost(n);
    xp -= need;
    l = n + 1.;
  }
  while (xp < 0 && l > 0) {
    int n = int(std::ceil(l)) - 1;
    double need = (l - n) * cost(n);
    if (-xp < need) return l + xp / cost(n);
    xp += need;
    l = n;
  }
  return l;
}
double LevelXP::value(const Curve& c) const {
  return level + progress / c.cost(level);
}
void LevelXP::set(double x, const Curve& c, int cap) {
  x = std::clamp(x, 0., double(cap));
  level = static_cast<std::uint16_t>(x);
  progress = level == cap ? 0 : float((x - level) * c.cost(level));
}
void LevelXP::add(double xp, const Curve& c, int cap) {
  if (!std::isfinite(xp) || xp <= 0) return;
  double p = progress + xp;
  while (level < cap && p >= c.cost(level)) {
    p -= c.cost(level);
    ++level;
  }
  progress = level >= cap ? 0 : float(p);
}
void LevelXP::subtract(double levels, const Curve& c, const LevelXP& f) {
  if (value(c) <= f.value(c)) return;
  set(std::max(f.value(c), value(c) - std::max(0., levels)), c, 65535);
}
double Sub::shape(double l) const {
  double x = T1 <= 0 ? 1 : std::clamp(l / T1, 0., 1.);
  double e = (l - T) / w;
  return e > 700
             ? 0
             : x * x * (3 - 2 * x) * (1 + std::exp(-T / w)) / (1 + std::exp(e));
}
void Sub::prepare(int cap) {
  integral.assign(cap + 1, 0);
  for (int i = 0; i < cap; ++i)
    integral[i + 1] = integral[i] + curve.cost(i) * shape(i + .5);
}
double Sub::worth(double l) const {
  l = std::clamp(l, 0., double(integral.size() - 1));
  auto n = size_t(l);
  return k * (integral[n] + (n + 1 < integral.size()
                                 ? (l - n) * (integral[n + 1] - integral[n])
                                 : 0));
}
void Skill::solve(int cap) {
  for (auto& s : subs) s.prepare(cap);
  for (auto& [g, b] : budgets) {
    double sum = 0;
    for (auto& s : subs)
      if (!s.fixed && s.group == g &&
          std::find(path.begin(), path.end(), s.id) != path.end()) {
        double saved = s.k;
        s.k = 1;
        sum += s.worth(pathLevel);
        s.k = saved;
      }
    if (sum <= 0)
      throw std::runtime_error(av + ": empty normalization path for " + g);
    for (auto& s : subs)
      if (!s.fixed && s.group == g) s.k = b / sum;
  }
}
double Skill::worthDelta() const {
  double x = 0;
  for (auto& s : subs)
    x += s.worth(
             std::max(s.current.value(s.curve), s.peak.value(s.curve) - rust)) -
         s.worth(s.initial);
  return x;
}
double Skill::computed(double lo, double hi) const {
  return std::clamp(curve.levelFromXP(base, worthDelta()), lo, hi);
}
double Settings::get(const char* s, const char* k) const {
  return values.at(s).at(k).get<double>();
}
bool Settings::flag(const char* s, const char* k) const {
  return values.at(s).at(k).get<bool>();
}
double hysteresis(double f, double n, double up, double down) {
  while (f >= n + up) ++n;
  while (f < n - down) --n;
  return n;
}
double ramp(double u, double g, double e) {
  return e <= g ? (u > g ? 1 : 0) : std::clamp((u - g) / (e - g), 0., 1.);
}
double rampIntegral(double u, double dt, double g, double e) {
  auto primitive = [&](double x) {
    if (e <= g) return std::max(0., x - g);
    double y = std::max(0., x - g), width = e - g;
    return y < width ? y * y / (2 * width) : y - width / 2;
  };
  return primitive(u + dt) - primitive(u);
}
void Model::initialize(Skill& sk, double b) {
  sk.base = b;
  sk.written = std::round(b);
  sk.rust = sk.idle = 0;
  sk.initialized = true;
  double f = std::clamp(settings.get("init", "floor_per_level") *
                            (b - settings.get("init", "floor_zero_at")),
                        0., settings.get("xp", "hard_cap"));
  for (auto& s : sk.subs) {
    s.initial = (s.fixed && s.k == 0) ? 0 : f;
    s.current.set(s.initial, s.curve, int(settings.get("xp", "hard_cap")));
    s.floor = s.potential = s.peak = s.current;
    s.idle = s.lastXP = 0;
  }
}
double Model::startLevel(double base) const {
  return std::clamp(settings.get("init", "floor_per_level") *
                        (base - settings.get("init", "floor_zero_at")),
                    0., settings.get("xp", "hard_cap"));
}
void Model::initializeShared(double best) {
  shared.base = shared.written = shared.rust = shared.idle = 0;
  shared.initialized = true;
  double f = startLevel(best);
  for (auto& s : shared.subs) {
    s.initial = s.group == "form" ? f : 0;
    s.current.set(s.initial, s.curve, int(settings.get("xp", "hard_cap")));
    s.floor = s.potential = s.peak = s.current;
    s.idle = s.lastXP = 0;
  }
}
std::vector<Skill*> Model::all() {
  std::vector<Skill*> out;
  for (auto& sk : skills) out.push_back(&sk);
  if (!shared.subs.empty()) out.push_back(&shared);
  return out;
}
std::vector<const Skill*> Model::all() const {
  std::vector<const Skill*> out;
  for (auto& sk : skills) out.push_back(&sk);
  if (!shared.subs.empty()) out.push_back(&shared);
  return out;
}
Sub* Model::find(const std::string& id, Skill** parent) {
  for (auto sk : all())
    for (auto& s : sk->subs)
      if (s.id == id) {
        if (parent) *parent = sk;
        return &s;
      }
  return nullptr;
}
void Model::award(const std::string& id, double raw, bool recover) {
  if (!std::isfinite(raw) || raw <= 0) return;
  Skill* sk = nullptr;
  Sub* s = find(id, &sk);
  if (!s || !sk->initialized) return;
  int cap = int(settings.get("xp", "hard_cap"));
  double left = raw, oldFloor = s->floor.value(s->curve),
         f = effectiveFloor(*s);  // the own floor moves below, effective floors do not
  // An effective floor (c × another floor) is not exactly representable as LevelXP:
  // setting current to it can land a hair below. Compare with a tolerance and bound the
  // number of zone steps so the loop always ends.
  for (int step = 0; left > 1e-10 && s->current.level < cap && step < 8; ++step) {
    double cur = s->current.value(s->curve),
           p = s->potential.value(s->curve), boundary = cap;
    double z = settings.get("xp", "zone_new");
    if (cur < f - 1e-6) {
      boundary = f;
      z = settings.get("xp", "zone_catch_up");
    } else if (cur < p - 1e-6) {
      boundary = p;
      z = settings.get("xp", "zone_recall");
    }
    double part = settings.flag("xp", "split_zone_gain")
                      ? std::min(left, s->curve.between(cur, boundary) / z)
                      : left;
    if (part <= 1e-10) {
      s->current.set(boundary, s->curve, cap);
      continue;
    }
    s->current.add(part * z, s->curve, cap);
    left -= part;
  }
  s->floor.add(raw * settings.get("xp", "floor_rate"), s->curve, cap);
  // No floor cap (design-v2 §5.4): the own floor only stays <= current.
  s->floor.set(std::max(oldFloor, std::min(s->floor.value(s->curve),
                                           s->current.value(s->curve))),
               s->curve, cap);
  if (s->current.value(s->curve) > s->potential.value(s->curve))
    s->potential = s->current;
  if (s->current.value(s->curve) > s->peak.value(s->curve))
    s->peak = s->current;
  double q = settings.get("decay", "q");
  s->idle = std::max(0., s->idle - raw / (q * sk->paceSub));
  s->lastXP = raw;
  if (recover) this->recover(*sk, raw);
}
// Rust and U_main fall once per vanilla award, by its raw XP, however many
// sub-skills that award feeds (a spell feeds three, a craft two).
void Model::recover(Skill& sk, double raw) {
  if (!std::isfinite(raw) || raw <= 0 || !sk.initialized || sk.pseudo) return;
  double q = settings.get("decay", "q");
  sk.idle = std::max(0., sk.idle - raw / (q * sk.paceSkill));
  sk.rust = std::max(0., sk.rust - sk.rustRate / 24 * raw / (q * sk.paceSkill));
}
void Model::tick(double hours) {
  double dt = hours * settings.get("debug", "time_scale"),
         g = settings.get("decay", "grace_hours"),
         e = settings.get("decay", "ramp_end_hours"),
         max = settings.get("decay", "idle_max_hours");
  if (settings.flag("debug", "no_grace")) g = e = 0;
  activeHours += hours;
  for (auto skp : all())
    if (auto& sk = *skp; sk.initialized) {
      double cap = 0;
      for (auto& s : sk.subs) {
        double amount = s.decay / 24 * rampIntegral(s.idle, dt, g, e);
        LevelXP stop;
        stop.set(effectiveFloor(s), s.curve, int(settings.get("xp", "hard_cap")));
        s.current.subtract(amount, s.curve, stop);
        s.idle = std::min(max, s.idle + dt);
        cap = std::max(cap, s.peak.value(s.curve) - s.floor.value(s.curve));
      }
      if (!sk.pseudo)
        sk.rust = std::min(
            cap, sk.rust + sk.rustRate / 24 * rampIntegral(sk.idle, dt, g, e));
      sk.idle = std::min(max, sk.idle + dt);
    }
}
static json lv(const LevelXP& l) { return json::array({l.level, l.progress}); }
json Model::save() const {
  json out = {{"version", 1},
              {"subs", unknownSubs},
              {"mains", unknownMains},
              {"activeHours", activeHours},
              {"lastHours", lastHours},
              {"pendingInit", pendingInit},
              {"raceClosed", raceClosed},
              {"requirements", json::object()}};
  for (auto& r : requirements) out["requirements"][r.handle] = r.met;
  for (auto skp : all()) {
    auto& sk = *skp;
    json initials =
        unknownMains.contains(sk.av)
            ? unknownMains.at(sk.av).value("initial", json::object())
            : json::object();
    for (auto& s : sk.subs) {
      out["subs"][s.id] = {{"current", lv(s.current)},
                           {"floor", lv(s.floor)},
                           {"potential", lv(s.potential)},
                           {"peak", lv(s.peak)},
                           {"idle", s.idle}};
      initials[s.id] = s.initial;
    }
    out["mains"][sk.av] = {{"initialized", sk.initialized},
                           {"base", sk.base},
                           {"rust", sk.rust},
                           {"idle", sk.idle},
                           {"written", sk.written},
                           {"initial", initials}};
  }
  return out;
}
void Model::restore(const json& j) {
  if (j.at("version") != 1)
    throw std::runtime_error("unsupported save version");
  auto read = [](const json& a) {
    return LevelXP{a.at(0).get<std::uint16_t>(), a.at(1).get<float>()};
  };
  unknownSubs = j.at("subs");
  unknownMains = j.at("mains");
  activeHours = j.at("activeHours");
  lastHours = j.at("lastHours");
  pendingInit = j.at("pendingInit");
  raceClosed = j.at("raceClosed");
  // Hysteresis state of perk requirements (absent in saves before prototype 2).
  auto met = j.value("requirements", json::object());
  for (auto& r : requirements) r.met = met.value(r.handle, false);
  // Forms and archetypes were per school before 2026-11-14 (`destruction/missile`,
  // `restoration/standard`); they are shared now (`form/missile`, `archetype/standard`).
  // Prototype saves only: drop the old records instead of carrying them as unknown.
  dropped.clear();
  std::set<std::string> sharedNames;
  for (auto& s : shared.subs) sharedNames.insert(s.id.substr(s.id.find('/') + 1));
  for (auto it = unknownSubs.begin(); it != unknownSubs.end();) {
    auto id = it.key();
    auto slash = id.find('/');
    bool old = slash != std::string::npos && !find(id) &&
               sharedNames.contains(id.substr(slash + 1)) &&
               id.substr(0, slash) != "form" && id.substr(0, slash) != "archetype";
    if (old) {
      dropped.push_back(id);
      for (auto& [av, main] : unknownMains.items())
        if (main.contains("initial")) main["initial"].erase(id);
      it = unknownSubs.erase(it);
    } else
      ++it;
  }
  for (auto skp : all()) {
    auto& sk = *skp;
    sk.initialized = false;
    if (unknownMains.contains(sk.av)) {
      auto v = unknownMains[sk.av];
      sk.initialized = v.at("initialized");
      sk.base = v.at("base");
      sk.rust = v.at("rust");
      sk.idle = v.at("idle");
      sk.written = v.at("written");
      for (auto& s : sk.subs) {
        if (!unknownSubs.contains(s.id) || !v.at("initial").contains(s.id)) {
          double f =
              (s.fixed && s.k == 0)
                  ? 0
                  : std::clamp(
                        settings.get("init", "floor_per_level") *
                            (sk.base - settings.get("init", "floor_zero_at")),
                        0., settings.get("xp", "hard_cap"));
          s.initial = f;
          s.current.set(f, s.curve, int(settings.get("xp", "hard_cap")));
          s.floor = s.potential = s.peak = s.current;
          s.idle = s.lastXP = 0;
          continue;
        }
        auto q = unknownSubs[s.id];
        s.current = read(q.at("current"));
        s.floor = read(q.at("floor"));
        s.potential = read(q.at("potential"));
        s.peak = read(q.at("peak"));
        s.idle = q.at("idle");
        s.initial = v["initial"][s.id];
        for (auto l : {s.current, s.floor, s.potential, s.peak})
          if (l.level > settings.get("xp", "hard_cap") ||
              !std::isfinite(l.progress) || l.progress < 0)
            throw std::runtime_error("invalid saved subskill " + s.id);
        if (!std::isfinite(s.idle) || s.idle < 0 || !std::isfinite(s.initial))
          throw std::runtime_error("invalid saved dynamics");
      }
      if (!std::isfinite(sk.base) || !std::isfinite(sk.rust) || sk.rust < 0 ||
          !std::isfinite(sk.idle) || sk.idle < 0 || !std::isfinite(sk.written))
        throw std::runtime_error("invalid saved main skill");
    }
  }
}
// Effect definitions for the panel tooltip (impl/status-ui.md *Effects and automatic perks*):
// the config's [[effect]] rows plus the aim effects of Aim.cpp, which the game applies.
json Model::effects(const Sub& s) const {
  json out = s.effects;
  auto aim = [&](const char* name, const char* at0, const char* zero,
                 const char* on) {
    out.push_back({{"name", name},
                   {"unit", "°"},
                   {"invert", false},
                   {"applied", settings.flag("aim", on)},
                   {"kind", "linear"},
                   {"at0", settings.get("aim", at0)},
                   {"zeroAt", settings.get("aim", zero)}});
  };
  if (s.group == "form" && s.id.ends_with("/missile"))
    aim("Launch noise σ", "missile_sigma_max_deg", "missile_L0",
        "missile_enabled");
  if (s.group == "form" && s.id.ends_with("/beam"))
    aim("Aim sway", "beam_amp_max_deg", "beam_L0", "beam_enabled");
  return out;
}
// Automatic perks with their state. Prototype: owned = current >= level (no perk is granted
// yet); with granting, owned = the player has the perk (hysteresis of design-v2 §5.6).
json Model::perks(const Sub& s, double current) const {
  json out = json::array();
  double keep = settings.get("perks", "hysteresis");
  for (auto p : s.perks) {
    double level = p.at("level");
    p["owned"] = current >= level;
    p["requires"] = json::array(
        {{{"sub", s.id}, {"level", level}, {"keep", std::max(0., level - keep)}}});
    out.push_back(p);
  }
  return out;
}
json Model::snapshot() const {
  json out ={{"state", pendingInit ? "pending" : "running"},
              {"activeHours", activeHours},
              {"debug", settings.values["debug"]},
              {"skills", json::array()},
              {"equipped", json::array()},
              {"requirements", requirementSnapshot()}};
  out["debug"]["write_main_skills"] =
      settings.flag("main", "write_main_skills");
  out["debugMode"] = settings.flag("ui", "debug_mode");
  double g = settings.get("decay", "grace_hours"),
         e = settings.get("decay", "ramp_end_hours");
  if (settings.flag("debug", "no_grace")) g = e = 0;
  for (auto skp : all()) {
    auto& sk = *skp;
    if (sk.pseudo) {
      // Pseudo-skill "magic": no level, no next-level bar, no main-skill line.
      json a = {{"av", "magic"}, {"name", sk.name}, {"pseudo", true},
                {"pending", !sk.initialized}, {"subs", json::array()}};
      for (auto& s : sk.subs) a["subs"].push_back(subSnapshot(sk, s, g, e));
      out["skills"].push_back(a);
      continue;
    }
    double F = sk.computed(settings.get("main", "min_level"),
                          settings.get("main", "max_level")),
           up = settings.get("main", "hysteresis_up");
    // Progress to the next written level: written n becomes n + 1 at F >= n + up.
    double toNext = sk.written >= settings.get("main", "max_level")
                        ? 1.
                        : std::clamp((F - sk.written) / up, 0., 1.);
    json a = {{"av", sk.av},
              {"name", sk.av == "OneHanded"   ? "One-Handed"
                       : sk.av == "TwoHanded" ? "Two-Handed"
                                              : sk.av},
              {"F", F},
              {"toNext", toNext},
              {"X", sk.worthDelta()},
              {"written", sk.written},
              {"base", sk.base},
              {"R", sk.rust},
              {"Umain", sk.idle},
              {"rustPerDay", sk.rustRate * ramp(sk.idle, g, e)},
              {"external", sk.external},
              {"pending", !sk.initialized},
              {"subs", json::array()}};
    for (auto& s : sk.subs) a["subs"].push_back(subSnapshot(sk, s, g, e));
    out["skills"].push_back(a);
  }
  return out;
}
json Model::subSnapshot(const Skill& sk, const Sub& s, double g, double e) const {
  std::string source;
  double c = s.current.value(s.curve), own = s.floor.value(s.curve),
         f = effectiveFloor(s, &source), p = s.potential.value(s.curve);
  return    {{"id", s.id},
           {"ownFloor", own},
           {"effFloor", f},
           {"floorFrom", source},
           {"current", c},
           {"potential", p},
           {"peak", s.peak.value(s.curve)},
           {"U", s.idle},
           {"decayPerDay", s.decay * ramp(s.idle, g, e)},
           {"zone", settings.get("xp", c < f   ? "zone_catch_up"
                                       : c < p ? "zone_recall"
                                               : "zone_new")},
           {"worth", s.worth(std::max(c, s.peak.value(s.curve) - sk.rust)) -
                         s.worth(s.initial)},
           {"lastXP", s.lastXP},
           {"effects", effects(s)},
           {"perks", perks(s, c)}};
}
const Sub& Model::at(const LinkIn& l) const {
  return l.skill < int(skills.size()) ? skills[l.skill].subs[l.sub] : shared.subs[l.sub];
}
double Model::mainLevel(const std::string& av) const {
  for (auto& sk : skills)
    if (sk.av == av) return sk.written;
  auto it = externalLevels.find(av);
  return it == externalLevels.end() ? 0 : it->second;
}
double Model::effectiveFloor(const Sub& s, std::string* source) const {
  double f = s.floor.value(s.curve);
  if (source) *source = "own";
  // Non-transitive (design-v2 §6.1): only own floors of the linked sub-skills count, and
  // only the largest term applies.
  for (auto& l : s.links) {
    auto& a = at(l);
    double v = l.c * a.floor.value(a.curve);
    if (v > f) {
      f = v;
      if (source) *source = a.id;
    }
  }
  if (s.magic && alteration.enabled) {
    double v = alteration.c * alteration.scale * mainLevel("Alteration");
    if (v > f) {
      f = v;
      if (source) *source = "Alteration";
    }
  }
  return std::min(f, settings.get("xp", "hard_cap"));
}
Values Model::values() const {
  return {[this](const std::string& id) {
            for (auto sk : all())
              for (auto& s : sk->subs)
                if (s.id == id) return s.current.value(s.curve);
            return 0.;
          },
          [this](const std::string& av) { return mainLevel(av); }};
}
std::vector<size_t> Model::updateRequirements() {
  std::vector<size_t> changed;
  auto v = values();
  double keep = settings.get("perk_requirements", "hysteresis");
  for (size_t i = 0; i < requirements.size(); ++i) {
    auto& r = requirements[i];
    bool now = evaluate(r.expr, v, r.met ? keep : 0);
    if (now != r.met) {
      r.met = now;
      changed.push_back(i);
    }
  }
  return changed;
}
json Model::requirementSnapshot() const {
  json out = json::array();
  auto v = values();
  double keep = settings.get("perk_requirements", "hysteresis");
  for (auto& r : requirements)
    out.push_back({{"handle", r.handle},
                   {"global", r.global},
                   {"formula", r.formula},
                   {"met", r.met},
                   {"explain", explain(r.expr, v, r.met ? keep : 0)}});
  return out;
}
}  // namespace sp
