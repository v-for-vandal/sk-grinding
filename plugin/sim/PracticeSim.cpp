// PracticeSim: the real sub-skill model (PracticeCore) without the game, for
// UI development and balance checks. Reads one JSON action per line on stdin,
// answers each with one line: the snapshot the DLL sends to the PrismaUI view
// (impl/status-ui.md), plus a "sim" object. Driven by tools/ui-sim/serve.py.
//
// It simulates the model only: no engine, no hooks, no context resolution. A
// "use" action names the sub-skills directly (what the router would pick).
#include <iostream>
#include <set>
#include <string>

#include "Core.h"
using namespace sp;
namespace {
struct Sim {
  std::string root;
  double base = 15;
  Model model;
  json equipped = json::array();
  json messages = json::array();
  double get(const char* s, const char* k) { return model.settings.get(s, k); }
  void initializeAll() {
    for (auto& sk : model.skills) model.initialize(sk, base);
    model.pendingInit = false;
    model.raceClosed = true;
    messages.push_back("all skills initialized at base " +
                       std::to_string(int(base)));
  }
  // Same rule as runtime::synchronize: written level with hysteresis.
  void synchronize() {
    for (auto& sk : model.skills) {
      if (!sk.initialized) continue;
      double f = sk.computed(get("main", "min_level"), get("main", "max_level"));
      double n = hysteresis(f, sk.written, get("main", "hysteresis_up"),
                            get("main", "hysteresis_down"));
      if (n != sk.written && model.settings.flag("main", "write_main_skills")) {
        messages.push_back(sk.av +
                           (n > sk.written ? " increased to " : " decreased to ") +
                           std::to_string(int(n)));
        sk.written = n;
      }
    }
  }
  Skill* skillByAV(const std::string& av) {
    for (auto& sk : model.skills)
      if (sk.av == av) return &sk;
    return nullptr;
  }
  // One vanilla award: every listed sub-skill gets raw × scale; rust of each
  // parent skill falls once (as runtime::route does).
  void use(const json& j) {
    double points = j.at("points");
    int times = j.value("times", 1);
    if (!std::isfinite(points) || points <= 0 || times < 1 || times > 100000)
      throw std::runtime_error("use: bad points/times");
    Skill* event = j.contains("skill") ? skillByAV(j.at("skill")) : nullptr;
    std::vector<std::pair<std::string, double>> subs;
    for (auto& s : j.at("subs")) {
      if (s.is_string())
        subs.emplace_back(s.get<std::string>(), 1.);
      else
        subs.emplace_back(s.at(0).get<std::string>(), s.at(1).get<double>());
    }
    if (subs.empty()) throw std::runtime_error("use: no sub-skills");
    if (!event) model.find(subs[0].first, &event);
    if (!event) throw std::runtime_error("use: unknown skill / sub-skill");
    double raw = points * event->baseMult * get("debug", "xp_mult");
    for (int t = 0; t < times; ++t) {
      std::vector<Skill*> parents;
      for (auto& [id, scale] : subs) {
        Skill* parent = nullptr;
        if (!model.find(id, &parent))
          throw std::runtime_error("use: unknown sub-skill " + id);
        model.award(id, raw * scale, false);
        if (std::find(parents.begin(), parents.end(), parent) == parents.end())
          parents.push_back(parent);
      }
      for (auto p : parents) model.recover(*p, raw);
      synchronize();
    }
  }
  void set(const json& j) {
    std::string section = j.at("section"), key = j.at("key");
    auto& values = model.settings.values;
    if (!values.contains(section) || !values[section].contains(key))
      throw std::runtime_error("set: unknown setting " + section + "." + key);
    values[section][key] = j.at("value");
  }
  json equippedSnapshot() {
    json out = json::array();
    for (auto e : equipped) {
      if (e.value("kind", "") == "spell") {
        std::string form = e.value("form", ""), school = e.value("school", "");
        auto sub = model.find(school + "/" + form);
        double level = sub ? sub->current.value(sub->curve) : 0;
        e["formLevel"] = level;
        e["sigma"] = form == "missile" && model.settings.flag("aim", "missile_enabled")
                         ? get("aim", "missile_sigma_max_deg") *
                               std::max(0., 1 - level / get("aim", "missile_L0"))
                         : 0.;
        // Amplitude after beam_t_full_sec of holding (the game shows the live value).
        e["sway"] = form == "beam" && model.settings.flag("aim", "beam_enabled")
                        ? get("aim", "beam_amp_max_deg") *
                              std::max(0., 1 - level / get("aim", "beam_L0"))
                        : 0.;
      }
      out.push_back(e);
    }
    return out;
  }
  json handle(const json& j) {
    std::string a = j.at("action");
    if (a == "snapshot") {
    } else if (a == "giveXP") {
      double raw = j.at("raw");
      if (!std::isfinite(raw) || raw < 0) throw std::runtime_error("giveXP: bad raw");
      model.award(j.at("id"), raw);  // one award: recovers rust itself
      synchronize();
    } else if (a == "setCurrent") {
      Skill* sk = nullptr;
      auto s = model.find(j.at("id"), &sk);
      double l = j.at("level");
      if (!s || !std::isfinite(l) || l < 0) throw std::runtime_error("setCurrent: bad id/level");
      s->current.set(l, s->curve, int(get("xp", "hard_cap")));
      if (s->current.value(s->curve) > s->potential.value(s->curve)) s->potential = s->current;
      if (s->current.value(s->curve) > s->peak.value(s->curve)) s->peak = s->current;
      synchronize();
    } else if (a == "use") {
      use(j);
    } else if (a == "tick") {
      double hours = j.at("hours");
      if (!std::isfinite(hours) || hours < 0 || hours > 24 * 365)
        throw std::runtime_error("tick: bad hours");
      // Small steps keep the rust cap (peak − floor) as close to the game's
      // 1 s ticks as needed; the decay integral itself is exact per step.
      for (double left = hours; left > 1e-12;) {
        double step = std::min(left, 0.5);
        model.tick(step);
        left -= step;
      }
      synchronize();
    } else if (a == "set") {
      set(j);
      synchronize();
    } else if (a == "equip") {
      equipped = j.at("equipped");
    } else if (a == "reinitAll") {
      if (j.contains("base")) base = j.at("base");
      initializeAll();
    } else if (a == "initNow") {
      messages.push_back("simulator: all skills are initialized at start");
    } else if (a == "reloadConfig") {
      model.loadConfig(root);
      for (auto& w : model.settings.values["configWarnings"]) messages.push_back(w);
      messages.push_back("config reloaded");
      synchronize();
    } else {
      messages.push_back("simulator: action '" + a + "' needs the game");
    }
    return {};
  }
  std::string snapshot(const std::string& error) {
    auto out = model.snapshot();
    out["debug"]["write_main_skills"] = model.settings.flag("main", "write_main_skills");
    out["equipped"] = equippedSnapshot();
    out["sim"] = {{"base", base},
                  {"messages", messages},
                  {"error", error},
                  {"aim", model.settings.values["aim"]},
                  {"debugSettings", model.settings.values["debug"]}};
    messages = json::array();
    return out.dump();
  }
};
}  // namespace
int main(int argc, char** argv) {
  std::ios::sync_with_stdio(false);
  Sim sim;
  sim.root = argc > 1 ? argv[1] : "config";
  try {
    if (argc > 2) sim.base = std::stod(argv[2]);
    sim.model.loadConfig(sim.root);
    sim.initializeAll();
  } catch (const std::exception& e) {
    std::cerr << "PracticeSim: " << e.what() << '\n';
    return 1;
  }
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    std::string error;
    try {
      sim.handle(json::parse(line));
    } catch (const std::exception& e) {
      error = e.what();
    }
    std::cout << sim.snapshot(error) << '\n' << std::flush;
  }
  return 0;
}
